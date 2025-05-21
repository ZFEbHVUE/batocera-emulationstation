#pragma once

#include "GuiComponent.h"
#include <vector>
#include <memory>
#include <string>
#include <utility>

class SwitchComponent;
class MenuComponent;
class ComponentGrid;
class Window;
class InputConfig;
struct Input;
struct Transform4x4f;

class GuiFavoriteMusicSelector : public GuiComponent
{
public:
    GuiFavoriteMusicSelector(Window* window);
    ~GuiFavoriteMusicSelector() override;

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const Transform4x4f& parentTrans) override;
    void onSizeChanged() override;

private:
    void loadAllMusic();
    void scanMusicDirectory(const std::string& path);
    void populateList();
    void saveFavorites();

    MenuComponent* mMenu;
    std::shared_ptr<ComponentGrid> mGrid;
    
    std::vector<std::pair<std::string, std::string>> mAllMusicFiles; // <path, filename>
    std::vector<std::shared_ptr<SwitchComponent>> mSwitches;
    
    Window* mWindow;
};


