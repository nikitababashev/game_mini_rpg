#include "Inventory.h"
#include <SDL3/SDL_log.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>

Inventory::Inventory() {
    slots.resize(SLOT_COUNT);
}

Inventory::~Inventory() {
    cleanup();
}

bool Inventory::init(SDL_Renderer* ren, TTF_Font* fnt, const std::string& slotTexturePath) {
    renderer = ren;
    font = fnt;

    if (!renderer || !font) {
        SDL_Log("Inventory::init: renderer or font is null");
        return false;
    }

    slotTexture = IMG_LoadTexture(renderer, slotTexturePath.c_str());
    if (!slotTexture) {
        SDL_Log("Inventory::init: failed to load slot texture from %s: %s",
            slotTexturePath.c_str(), SDL_GetError());
        return false;
    }

    selectedSlotTexture = createSelectedSlotTexture();
    if (!selectedSlotTexture) {
        selectedSlotTexture = slotTexture;
    }

    // Создаём фон панели
    int bgWidth = (int)(SLOT_COUNT * (SLOT_SIZE + SLOT_PADDING) + SLOT_PADDING * 2);
    int bgHeight = (int)BAR_HEIGHT;

    SDL_Surface* bgSurface = SDL_CreateSurface(bgWidth, bgHeight, SDL_PIXELFORMAT_ARGB8888);
    if (bgSurface) {
        SDL_FillSurfaceRect(bgSurface, nullptr, SDL_MapSurfaceRGBA(bgSurface, 20, 20, 30, 200));
        slotBgTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
        SDL_DestroySurface(bgSurface);
    }

    for (int i = 0; i < SLOT_COUNT; ++i) {
        slots[i].itemName = "";
        slots[i].icon = nullptr;
        slots[i].count = 0;
    }

    selectedSlot = 0;
    return true;
}

SDL_Texture* Inventory::createSelectedSlotTexture() {
    if (!slotTexture) return nullptr;

    float texW, texH;
    SDL_GetTextureSize(slotTexture, &texW, &texH);

    SDL_Texture* targetTex = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_TARGET,
        (int)texW, (int)texH
    );
    if (!targetTex) return nullptr;

    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, targetTex);

    SDL_FRect rect = { 0, 0, texW, texH };
    SDL_RenderTexture(renderer, slotTexture, nullptr, &rect);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 180);

    SDL_FRect borderRect = { 4, 4, texW - 8, texH - 8 };
    SDL_RenderRect(renderer, &borderRect);

    float cornerSize = 15.0f;
    SDL_FRect corner1 = { 2, 2, cornerSize, 4 };
    SDL_RenderFillRect(renderer, &corner1);
    SDL_FRect corner1b = { 2, 2, 4, cornerSize };
    SDL_RenderFillRect(renderer, &corner1b);

    SDL_FRect corner2 = { texW - cornerSize - 2, 2, cornerSize, 4 };
    SDL_RenderFillRect(renderer, &corner2);
    SDL_FRect corner2b = { texW - 6, 2, 4, cornerSize };
    SDL_RenderFillRect(renderer, &corner2b);

    SDL_FRect corner3 = { 2, texH - 6, cornerSize, 4 };
    SDL_RenderFillRect(renderer, &corner3);
    SDL_FRect corner3b = { 2, texH - cornerSize - 2, 4, cornerSize };
    SDL_RenderFillRect(renderer, &corner3b);

    SDL_FRect corner4 = { texW - cornerSize - 2, texH - 6, cornerSize, 4 };
    SDL_RenderFillRect(renderer, &corner4);
    SDL_FRect corner4b = { texW - 6, texH - cornerSize - 2, 4, cornerSize };
    SDL_RenderFillRect(renderer, &corner4b);

    SDL_SetRenderTarget(renderer, oldTarget);

    return targetTex;
}

void Inventory::cleanup() {
    if (slotTexture && slotTexture != selectedSlotTexture) {
        SDL_DestroyTexture(slotTexture);
    }
    if (selectedSlotTexture && selectedSlotTexture != slotTexture) {
        SDL_DestroyTexture(selectedSlotTexture);
    }
    if (slotBgTexture) {
        SDL_DestroyTexture(slotBgTexture);
    }

    slotTexture = nullptr;
    selectedSlotTexture = nullptr;
    slotBgTexture = nullptr;

    for (auto& nt : numberCache) {
        if (nt.texture) {
            SDL_DestroyTexture(nt.texture);
        }
    }
    numberCache.clear();

    slots.clear();
}

SDL_Texture* Inventory::renderNumber(int number) {
    std::string numStr = std::to_string(number);
    SDL_Color color = { 255, 255, 255, 255 };
    SDL_Surface* surf = TTF_RenderText_Blended(font, numStr.c_str(), 0, color);
    if (!surf) {
        SDL_Log("Failed to render number %d: %s", number, SDL_GetError());
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);
    return texture;
}

void Inventory::updateNumberCache() {
    for (auto& nt : numberCache) {
        if (nt.texture) {
            SDL_DestroyTexture(nt.texture);
        }
    }
    numberCache.clear();

    std::vector<int> counts;
    for (const auto& slot : slots) {
        if (slot.count > 1) {
            bool exists = false;
            for (int c : counts) {
                if (c == slot.count) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                counts.push_back(slot.count);
            }
        }
    }

    for (int count : counts) {
        SDL_Texture* tex = renderNumber(count);
        if (tex) {
            numberCache.push_back({ tex, count });
        }
    }
}

void Inventory::drawItemCount(int slotIndex, int count, float x, float y, float slotSize) {
    if (count <= 1) return;

    SDL_Texture* numberTex = nullptr;
    for (const auto& nt : numberCache) {
        if (nt.number == count) {
            numberTex = nt.texture;
            break;
        }
    }

    if (!numberTex) {
        numberTex = renderNumber(count);
        if (!numberTex) return;
    }

    float texW, texH;
    SDL_GetTextureSize(numberTex, &texW, &texH);

    float textX = x + slotSize - texW - 8.0f;
    float textY = y + slotSize - texH - 6.0f;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect shadowRect = { textX + 1, textY + 1, texW, texH };
    SDL_RenderFillRect(renderer, &shadowRect);

    SDL_FRect dest = { textX, textY, texW, texH };
    SDL_RenderTexture(renderer, numberTex, nullptr, &dest);

    if (numberTex != nullptr && std::find_if(numberCache.begin(), numberCache.end(),
        [numberTex](const NumberTexture& nt) { return nt.texture == numberTex; }) == numberCache.end()) {
        SDL_DestroyTexture(numberTex);
    }
}

void Inventory::draw(float screenWidth, float screenHeight) {
    if (!renderer || !slotTexture) return;

    updateNumberCache();

    float totalWidth = SLOT_COUNT * SLOT_SIZE + (SLOT_COUNT - 1) * SLOT_PADDING;
    float startX = (screenWidth - totalWidth) / 2.0f;
    float startY = screenHeight - BAR_HEIGHT - BOTTOM_OFFSET;

    if (slotBgTexture) {
        float bgWidth = totalWidth + SLOT_PADDING * 2;
        float bgHeight = BAR_HEIGHT;
        float bgX = (screenWidth - bgWidth) / 2.0f;
        float bgY = startY - SLOT_PADDING;
        SDL_FRect bgDest = { bgX, bgY, bgWidth, bgHeight };
        SDL_RenderTexture(renderer, slotBgTexture, nullptr, &bgDest);
    }

    for (int i = 0; i < SLOT_COUNT; ++i) {
        float x = startX + i * (SLOT_SIZE + SLOT_PADDING);
        float y = startY;

        SDL_FRect dest = { x, y, SLOT_SIZE, SLOT_SIZE };

        SDL_Texture* currentSlotTex = (i == selectedSlot) ? selectedSlotTexture : slotTexture;
        if (currentSlotTex) {
            SDL_RenderTexture(renderer, currentSlotTex, nullptr, &dest);
        }

        if (!slots[i].isEmpty() && slots[i].icon) {
            float iconPadding = 12.0f;
            float iconSize = SLOT_SIZE - iconPadding * 2;
            SDL_FRect iconDest = { x + iconPadding, y + iconPadding, iconSize, iconSize };
            SDL_RenderTexture(renderer, slots[i].icon, nullptr, &iconDest);
            drawItemCount(i, slots[i].count, x, y, SLOT_SIZE);
        }
    }

    if (selectedSlot >= 0 && selectedSlot < SLOT_COUNT && !slots[selectedSlot].isEmpty() && font) {
        float texW, texH;
        std::string hint = slots[selectedSlot].itemName;
        if (slots[selectedSlot].count > 1) {
            hint += " x" + std::to_string(slots[selectedSlot].count);
        }

        SDL_Color color = { 255, 255, 255, 220 };
        SDL_Surface* surf = TTF_RenderText_Blended(font, hint.c_str(), 0, color);
        if (surf) {
            SDL_Texture* hintTex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_GetTextureSize(hintTex, &texW, &texH);

            float hintBgPadding = 8.0f;
            float hintX = startX + selectedSlot * (SLOT_SIZE + SLOT_PADDING) + (SLOT_SIZE - texW) / 2.0f;
            float hintY = startY - texH - 15.0f;

            SDL_FRect hintBgDest = { hintX - hintBgPadding, hintY - hintBgPadding / 2,
                                      texW + hintBgPadding * 2, texH + hintBgPadding };
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_RenderFillRect(renderer, &hintBgDest);
            SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
            SDL_RenderRect(renderer, &hintBgDest);

            SDL_FRect hintDest = { hintX, hintY, texW, texH };
            SDL_RenderTexture(renderer, hintTex, nullptr, &hintDest);
            SDL_DestroyTexture(hintTex);
            SDL_DestroySurface(surf);
        }
    }
}

void Inventory::update() {
    // Обновление инвентаря
}

bool Inventory::addItem(const std::string& itemName, SDL_Texture* icon, int count) {
    if (count <= 0) return false;

    int slotIndex = findSlotByItemName(itemName);
    if (slotIndex != -1) {
        slots[slotIndex].count += count;
        SDL_Log("Inventory::addItem: Added %d %s to existing slot, new count: %d", count, itemName.c_str(), slots[slotIndex].count);
        return true;
    }

    slotIndex = findFirstEmptySlot();
    if (slotIndex != -1) {
        slots[slotIndex].itemName = itemName;
        slots[slotIndex].icon = icon;
        slots[slotIndex].count = count;
        SDL_Log("Inventory::addItem: Added %d %s to empty slot %d", count, itemName.c_str(), slotIndex);
        return true;
    }

    SDL_Log("Inventory::addItem: Inventory is full! Cannot add %s", itemName.c_str());
    return false;
}

bool Inventory::removeItem(const std::string& itemName, int count) {
    int slotIndex = findSlotByItemName(itemName);
    if (slotIndex == -1) {
        SDL_Log("Inventory::removeItem: Item '%s' not found", itemName.c_str());
        return false;
    }

    if (slots[slotIndex].count >= count) {
        slots[slotIndex].count -= count;
        SDL_Log("Inventory::removeItem: Removed %d %s, remaining: %d", count, itemName.c_str(), slots[slotIndex].count);

        if (slots[slotIndex].count <= 0) {
            slots[slotIndex].itemName = "";
            slots[slotIndex].icon = nullptr;
            slots[slotIndex].count = 0;
            SDL_Log("Inventory::removeItem: Slot %d is now empty", slotIndex);
        }
        return true;
    }

    SDL_Log("Inventory::removeItem: Not enough %s (have %d, need %d)", itemName.c_str(), slots[slotIndex].count, count);
    return false;
}

bool Inventory::hasItem(const std::string& itemName, int count) const {
    int slotIndex = findSlotByItemName(itemName);
    if (slotIndex == -1) return false;
    return slots[slotIndex].count >= count;
}

int Inventory::getItemCount(const std::string& itemName) const {
    int slotIndex = findSlotByItemName(itemName);
    if (slotIndex == -1) return 0;
    return slots[slotIndex].count;
}

void Inventory::clear() {
    for (int i = 0; i < SLOT_COUNT; ++i) {
        slots[i].itemName = "";
        slots[i].icon = nullptr;
        slots[i].count = 0;
    }
    selectedSlot = 0;
}

void Inventory::setSelectedSlot(int slotIndex) {
    if (slotIndex >= 0 && slotIndex < SLOT_COUNT) {
        selectedSlot = slotIndex;
        SDL_Log("Inventory: Selected slot %d", slotIndex);
    }
}

const InventorySlot* Inventory::getSelectedItem() const {
    if (selectedSlot >= 0 && selectedSlot < SLOT_COUNT && !slots[selectedSlot].isEmpty()) {
        return &slots[selectedSlot];
    }
    return nullptr;
}

int Inventory::findSlotByItemName(const std::string& itemName) const {
    for (int i = 0; i < SLOT_COUNT; ++i) {
        if (slots[i].itemName == itemName && slots[i].count > 0) {
            return i;
        }
    }
    return -1;
}

int Inventory::findFirstEmptySlot() const {
    for (int i = 0; i < SLOT_COUNT; ++i) {
        if (slots[i].isEmpty()) {
            return i;
        }
    }
    return -1;
}