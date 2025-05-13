#include "guis/GuiFavoriteMusicSelector.h"
#include "guis/GuiSettings.h"
#include "guis/GuiMsgBox.h"
#include "components/SwitchComponent.h"
#include "components/TextComponent.h"
#include "components/ButtonComponent.h"
#include "FavoriteMusicManager.h"
#include "Settings.h"
#include "AudioManager.h"
#include "Paths.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"
#include "LocaleES.h"
#include "renderers/Renderer.h"
#include <fstream>
#include <set>
#include <algorithm>

void GuiFavoriteMusicSelector::openSelectFavoriteSongs(Window *window, bool browseMusicMode, bool animate, 
                                                    const std::string& customPath)
{
    // Use appropriate title based on mode
    auto s = new GuiSettings(window, (browseMusicMode ? _("SELECTION FAVORITE SONG") : _("FAVORITES LIST")).c_str());
    s->setCloseButton("select");
    
    // Load existing favorites
    std::vector<std::pair<std::string, std::string>> favorites;
    std::set<std::string> favoriteSet;
    std::vector<std::pair<std::string, std::string>> directoryFiles;
    std::vector<std::shared_ptr<SwitchComponent>> directorySwitches;
    std::vector<std::shared_ptr<SwitchComponent>> favoritesSwitches;
    
    if (Utils::FileSystem::exists(FavoriteMusicManager::getFavoriteMusicFilePath()))
    {
        favorites = FavoriteMusicManager::loadFavoriteSongs(
            FavoriteMusicManager::getFavoriteMusicFilePath());
        
        // Create a set for quick lookups
        for (auto& fav : favorites)
        {
            favoriteSet.insert(fav.first);
        }
    }
    
    // Determine base or custom path
    std::string currentPath;
    if (!customPath.empty()) {
        currentPath = customPath;
    } else {
        // Use default base path
        currentPath = Paths::getUserMusicPath();
        if (currentPath.empty())
            currentPath = Paths::getMusicPath();
    }
    
    // If we're in browse music mode, show the first section
    if (browseMusicMode)
    {
        s->addGroup(_("SELECTION FAVORITE SONG"));
        
        // Add parent directory option if not at root
        std::string basePath = Paths::getUserMusicPath();
        if (basePath.empty())
            basePath = Paths::getMusicPath();
            
        if (currentPath != basePath)
        {
            std::string parentPath = Utils::FileSystem::getParent(currentPath);
            s->addEntry(_("PARENT DIRECTORY"), false, [window, parentPath, animate]() {
                // Open a new selector with the parent directory
                GuiFavoriteMusicSelector::openSelectFavoriteSongs(window, true, animate, parentPath);
            }, "iconUpArrow");
        }
        
        // Get and display folders
        std::vector<std::string> directories;
        if (Utils::FileSystem::exists(currentPath))
        {
            auto dirContentList = Utils::FileSystem::getDirContent(currentPath, false);
            
            // Filter only directories
            for (auto& entry : dirContentList)
            {
                if (Utils::FileSystem::isDirectory(entry))
                    directories.push_back(entry);
            }
            
            // Sort alphabetically
            std::sort(directories.begin(), directories.end(), [](const std::string& a, const std::string& b) {
                return Utils::String::toUpper(Utils::FileSystem::getFileName(a)) < 
                       Utils::String::toUpper(Utils::FileSystem::getFileName(b));
            });
            
            // Add the directories
            for (auto& dir : directories)
            {
                std::string name = Utils::FileSystem::getFileName(dir);
                
                s->addEntry(name, false, [window, dir, animate]() {
                    // Recursive call with the new directory
                    GuiFavoriteMusicSelector::openSelectFavoriteSongs(window, true, animate, dir);
                }, "iconFolder");
            }
        }

        // Get and display audio files
        std::vector<std::string> files;
        if (Utils::FileSystem::exists(currentPath))
        {
            auto dirContentList = Utils::FileSystem::getDirContent(currentPath, false);
            
            // Filter only audio files
            for (auto& entry : dirContentList)
            {
                if (Utils::FileSystem::isAudio(entry))
                    files.push_back(entry);
            }
            
            // Sort alphabetically
            std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
                return Utils::String::toUpper(Utils::FileSystem::getFileName(a)) < 
                       Utils::String::toUpper(Utils::FileSystem::getFileName(b));
            });
            
            // Add the audio files
            for (auto& file : files)
            {
                std::string fullName = Utils::FileSystem::getFileName(file);
                
                // Remove extension for compatibility with QuickAccess
                std::string nameWithoutExt = fullName;
                size_t lastDot = fullName.find_last_of('.');
                if (lastDot != std::string::npos) {
                    nameWithoutExt = fullName.substr(0, lastDot);
                }
                
                // Check if this file is already in favorites
                bool isInFavorites = false;
                for (auto& fav : favorites) {
                    if (fav.first == file) {
                        isInFavorites = true;
                        break;
                    }
                }
                
                // Create switch with current state
                auto sw = std::make_shared<SwitchComponent>(window);
                sw->setState(isInFavorites);
                
                // Add the entry (show full name for UI)
                s->addWithDescription(fullName, "", sw, nullptr, "iconSound");
                
                // Store file and its name WITHOUT extension for saving
                directoryFiles.emplace_back(file, nameWithoutExt);
                directorySwitches.push_back(sw);
            }
        }
    }
    
    // Display the favorites section
    s->addGroup(_("FAVORITE FILE"));
    
    // Only display if favorites exist
    if (!favorites.empty())
    {
        // Display all favorites with simple layout
        for (auto& fav : favorites)
        {
            std::string name = fav.second;
            std::string path = fav.first;
            
            // Create a switch enabled by default
            auto sw = std::make_shared<SwitchComponent>(window);
            sw->setState(true);  // All enabled by default
            
            // Add with simplified display
            //s->addWithDescription(name, "", sw, nullptr, "iconSound");
            s->addWithDescription(name, path, sw, nullptr, "iconSound");
            
            // Store the switch
            favoritesSwitches.push_back(sw);
        }
    }
    else
    {
        // Message if there are no favorites
        s->addEntry(_("NO FAVORITES FOUND"), false, nullptr, "iconInfo");
    }
    
    // Add saveFunc to handle favorites when exiting
    s->addSaveFunc([window, favorites, favoritesSwitches, directoryFiles, directorySwitches]() {
        // Create favorites directory if needed
        auto favoritePath = FavoriteMusicManager::getFavoriteMusicPath();
        if (!Utils::FileSystem::exists(favoritePath))
            Utils::FileSystem::createDirectory(favoritePath);
        
        // Handle removals - for any existing favorite that is now OFF
        for (size_t i = 0; i < favorites.size(); ++i) {
            if (!favoritesSwitches[i]->getState()) {
                // This favorite should be removed - use FavoriteMusicManager method
                FavoriteMusicManager::getInstance().removeSongFromFavorites(
                    favorites[i].first, favorites[i].second, window);
            }
        }
        
        // Handle additions - for any file that is ON but not already in favorites
        for (size_t i = 0; i < directoryFiles.size(); ++i) {
            if (directorySwitches[i]->getState()) {
                std::string filePath = directoryFiles[i].first;
                std::string fileName = directoryFiles[i].second; // Name without extension
                
                // Check if it's not already in favorites
                bool found = false;
                for (auto& fav : favorites) {
                    if (fav.first == filePath) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    // Add using FavoriteMusicManager method
                    FavoriteMusicManager::getInstance().saveSongToFavorites(filePath, fileName, window);
                }
            }
        }
    });
    
    // Animation like in QuickAccess
    if (animate)
        s->getMenu().animateTo(Vector2f((Renderer::getScreenWidth() - s->getMenu().getSize().x()) / 2, 
                                       (Renderer::getScreenHeight() - s->getMenu().getSize().y()) / 2));
    else
        s->getMenu().setPosition((Renderer::getScreenWidth() - s->getMenu().getSize().x()) / 2, 
                                (Renderer::getScreenHeight() - s->getMenu().getSize().y()) / 2);
    
    window->pushGui(s);
}
