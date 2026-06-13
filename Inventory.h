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

    // Инициализация
    bool init(SDL_Renderer* renderer, TTF_Font* font, const std::string& slotTexturePath);
    void cleanup();

    // Управление предметами
    bool addItem(const std::string& itemName, SDL_Texture* icon, int count = 1);
    bool removeItem(const std::string& itemName, int count = 1);
    bool hasItem(const std::string& itemName, int count = 1) const;
    int getItemCount(const std::string& itemName) const;
    void clear();

    // Выделение слота
    void setSelectedSlot(int slotIndex);
    int getSelectedSlot() const { return selectedSlot; }

    // Отрисовка
    void draw(float screenWidth, float screenHeight);

    // Обновление
    void update();

    // Получить выбранный предмет
    const InventorySlot* getSelectedItem() const;

private:
    static constexpr int SLOT_COUNT = 8;
    static constexpr float SLOT_SIZE = 80.0f;
    static constexpr float SLOT_PADDING = 12.0f;
    static constexpr float BAR_HEIGHT = 100.0f;
    static constexpr float BOTTOM_OFFSET = 20.0f;

    std::vector<InventorySlot> slots;
    int selectedSlot = 0;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;

    // Текстуры для UI
    SDL_Texture* slotTexture = nullptr;
    SDL_Texture* selectedSlotTexture = nullptr;
    SDL_Texture* slotBgTexture = nullptr;

    // Текстуры для чисел
    struct NumberTexture {
        SDL_Texture* texture;
        int number;
    };
    std::vector<NumberTexture> numberCache;

    SDL_Texture* renderNumber(int number);
    void drawItemCount(int slotIndex, int count, float x, float y, float slotSize);

    // Вспомогательные методы
    int findSlotByItemName(const std::string& itemName) const;
    int findFirstEmptySlot() const;
    void updateNumberCache();

    // Создание текстуры выделенного слота
    SDL_Texture* createSelectedSlotTexture();
};