#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <functional>
#include <SDL3_ttf/SDL_ttf.h>

struct InventorySlot {
    std::string itemName;
    SDL_Texture* icon = nullptr;
    int count = 0;
    bool isEmpty() const { return count <= 0 || itemName.empty(); }
};

class Inventory {
public:
    Inventory();
    ~Inventory();

    bool init(SDL_Renderer* renderer, TTF_Font* font, const std::string& slotTexturePath);
    void cleanup();

    bool addItem(const std::string& itemName, SDL_Texture* icon, int count = 1);
    bool removeItem(const std::string& itemName, int count = 1);
    bool hasItem(const std::string& itemName, int count = 1) const;
    int getItemCount(const std::string& itemName) const;
    void clear();

    void setSelectedSlot(int slotIndex);
    int getSelectedSlot() const { return selectedSlot; }

    void draw(float screenWidth, float screenHeight);

    void update();

    void setUseItemCallback(std::function<void(const std::string&, int)> callback);

    const InventorySlot* getSelectedItem() const;

private:
    static constexpr int SLOT_COUNT = 8;
    static constexpr float SLOT_SIZE = 80.0f;      // Размер ячейки
    static constexpr float SLOT_PADDING = 12.0f;    // Расстояние между ячейками
    static constexpr float BAR_HEIGHT = 100.0f;     // Высота панели
    static constexpr float BOTTOM_OFFSET = 20.0f;   // Отступ от низа

    std::vector<InventorySlot> slots;
    int selectedSlot = 0;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;

    SDL_Texture* slotTexture = nullptr;       
    SDL_Texture* selectedSlotTexture = nullptr; 
    SDL_Texture* slotBgTexture = nullptr;      // Фон панели инвентаря
    // Текстуры для чисел (количество предметов)
    struct NumberTexture {
        SDL_Texture* texture;
        int number;
    };
    std::vector<NumberTexture> numberCache;

    SDL_Texture* renderNumber(int number);
    void drawItemCount(int slotIndex, int count, float x, float y, float slotSize);

    std::function<void(const std::string&, int)> onUseItem;

    int findSlotByItemName(const std::string& itemName) const;
    int findFirstEmptySlot() const;
    void updateNumberCache();

    SDL_Texture* createSelectedSlotTexture();
};