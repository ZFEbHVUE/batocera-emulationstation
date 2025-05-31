#pragma once

#include "GuiComponent.h"
#include <vector>
#include <memory>
#include <string>

class MenuComponent;
class SwitchComponent;
class Window;

class GuiFavoriteMusicSelector : public GuiComponent
{
public:
    GuiFavoriteMusicSelector(Window* window);
    ~GuiFavoriteMusicSelector();
    
    static void openSelectFavoriteSongs(Window* window, bool = false, bool = false);

private:
    void loadMusic();
    void save();
    
    MenuComponent* mMenu;
    std::vector<std::pair<std::string, std::string>> mFiles; // path, name
    std::vector<std::shared_ptr<SwitchComponent>> mSwitches;
};

