#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <set>

class Window;
class SwitchComponent;

class GuiFavoriteMusicSelector
{
public:
    // Static method for opening the favorite music selector menu
    static void openSelectFavoriteSongs(Window *window, bool browseMusicMode, bool animate, 
                                      const std::string& customPath = "");
    
private:
    // Constructor is private to prevent instantiation
    GuiFavoriteMusicSelector() {}
};
