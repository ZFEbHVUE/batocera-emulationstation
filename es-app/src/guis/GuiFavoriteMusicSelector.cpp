#include "guis/GuiFavoriteMusicSelector.h"
#include "components/SwitchComponent.h"
#include "components/MenuComponent.h"
#include "components/ComponentGrid.h"
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
#include "Window.h"
#include <fstream>
#include <set>
#include <algorithm>

GuiFavoriteMusicSelector::GuiFavoriteMusicSelector(Window* window)
    : GuiComponent(window), mWindow(window)
{
    // Create layout
    mMenu = new MenuComponent(window, _("SELECTION OF FAVORITE SONGS"));

    // Bottom buttons
    auto saveBtn = std::make_shared<ButtonComponent>(mWindow, _("SAVE"), _("SAVE"), [this] { 
        saveFavorites(); 
        delete this;
    });

    auto cancelBtn = std::make_shared<ButtonComponent>(mWindow, _("CANCEL"), _("CANCEL"), [this] {
        delete this;
    });

    // Simple 2-button grid at bottom
    mGrid = std::make_shared<ComponentGrid>(window, Vector2i(2, 1));
    mGrid->setEntry(saveBtn, Vector2i(0, 0), true, false);
    mGrid->setEntry(cancelBtn, Vector2i(1, 0), true, false);

    // Load all music and populate the list
    loadAllMusic();
    populateList();
        
    addChild(mMenu);
    addChild(mGrid.get());

    // Set size based on screen dimensions
    setSize(Renderer::getScreenWidth() * 0.85f, Renderer::getScreenHeight() * 0.85f);
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2,
                (Renderer::getScreenHeight() - mSize.y()) / 2);
}

GuiFavoriteMusicSelector::~GuiFavoriteMusicSelector()
{
    delete mMenu;
}

void GuiFavoriteMusicSelector::openSelectFavoriteSongs(Window* window, bool browseMusicMode, bool quickAccess)
{
    window->pushGui(new GuiFavoriteMusicSelector(window));
}

void GuiFavoriteMusicSelector::onSizeChanged()
{
    float gridHeight = 60; // Fixed height for buttons
    
    float menuHeight = mSize.y() - gridHeight - 10;
    if (menuHeight < 0) menuHeight = 0;
    
    mMenu->setSize(mSize.x(), menuHeight);
    mMenu->setPosition(0, 0);
    
    mGrid->setSize(mSize.x(), gridHeight);
    mGrid->setPosition(0, menuHeight + 10);
}

void GuiFavoriteMusicSelector::loadAllMusic()
{
    mAllMusicFiles.clear();
    
    // Get all music from both user and default directories
    std::vector<std::string> musicPaths;
    
    if (!Paths::getUserMusicPath().empty())
        musicPaths.push_back(Paths::getUserMusicPath());
    
    if (!Paths::getMusicPath().empty())
        musicPaths.push_back(Paths::getMusicPath());
    
    // Scan all directories recursively
    for (const std::string& path : musicPaths)
    {
        scanMusicDirectory(path);
    }
    
    // Sort alphabetically by filename
    std::sort(mAllMusicFiles.begin(), mAllMusicFiles.end(), 
              [](const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b) {
                  return Utils::String::toUpper(a.second) < Utils::String::toUpper(b.second);
              });
}

void GuiFavoriteMusicSelector::scanMusicDirectory(const std::string& path)
{
    if (!Utils::FileSystem::exists(path) || !Utils::FileSystem::isDirectory(path))
        return;
    
    auto dirContent = Utils::FileSystem::getDirContent(path, false);
    
    for (auto& entry : dirContent)
    {
        if (Utils::FileSystem::isDirectory(entry))
        {
            // Recursively scan subdirectories
            scanMusicDirectory(entry);
        }
        else if (Utils::FileSystem::isAudio(entry))
        {
            std::string fileName = Utils::FileSystem::getFileName(entry);
            mAllMusicFiles.emplace_back(entry, fileName);
        }
    }
}

void GuiFavoriteMusicSelector::populateList()
{
    // Load current favorites
    std::set<std::string> favoritePaths;
    if (Utils::FileSystem::exists(FavoriteMusicManager::getFavoriteMusicFilePath()))
    {
        auto favorites = FavoriteMusicManager::loadFavoriteSongs(FavoriteMusicManager::getFavoriteMusicFilePath());
        for (auto& fav : favorites)
            favoritePaths.insert(fav.first);
    }
    
    // Clear existing entries
    mMenu->clear();
    mSwitches.clear();
    
    // Add all music files with checkboxes
    for (auto& musicFile : mAllMusicFiles)
    {
        auto sw = std::make_shared<SwitchComponent>(mWindow);
        sw->setState(favoritePaths.count(musicFile.first) > 0);
        
        mMenu->addWithLabel(musicFile.second, sw);
        mSwitches.push_back(sw);
    }
}

void GuiFavoriteMusicSelector::saveFavorites()
{
    auto favoritePath = FavoriteMusicManager::getFavoriteMusicPath();
    if (!Utils::FileSystem::exists(favoritePath))
        Utils::FileSystem::createDirectory(favoritePath);
    
    auto favFile = FavoriteMusicManager::getFavoriteMusicFilePath();
    
    // Prepare the new favorites list
    std::vector<std::pair<std::string, std::string>> newFavorites;
    
    // Add selected items to favorites
    for (size_t i = 0; i < mAllMusicFiles.size() && i < mSwitches.size(); ++i)
    {
        if (mSwitches[i]->getState())
        {
            newFavorites.push_back(mAllMusicFiles[i]);
        }
    }
    
    // Save the favorites list
    if (!newFavorites.empty())
    {
        std::ofstream ofs(favFile, std::ios::trunc);
        if (ofs.is_open())
        {
            for (auto& fav : newFavorites)
            {
                ofs << fav.first << ";" << fav.second << "\n";
            }
            ofs.close();
        }
    }
    else if (Utils::FileSystem::exists(favFile))
    {
        // No favorites selected, remove the file
        Utils::FileSystem::removeFile(favFile);
    }
}

bool GuiFavoriteMusicSelector::input(InputConfig* config, Input input)
{
    if (mMenu->input(config, input))
        return true;
    
    if (mGrid->input(config, input))
        return true;
    
    // Add hotkey for toggling favorite status (e.g., Y button)
    if (input.value != 0 && config->isMappedTo("y", input))
    {
        int idx = mMenu->getCursorIndex();
        if (idx >= 0 && idx < mSwitches.size())
        {
            mSwitches[idx]->setState(!mSwitches[idx]->getState());
            return true;
        }
    }
    
    return GuiComponent::input(config, input);
}

void GuiFavoriteMusicSelector::update(int deltaTime)
{
    GuiComponent::update(deltaTime);
    mMenu->update(deltaTime);
    mGrid->update(deltaTime);
}

void GuiFavoriteMusicSelector::render(const Transform4x4f& parentTrans)
{
    GuiComponent::render(parentTrans);
    mMenu->render(parentTrans);
    mGrid->render(parentTrans);
}

