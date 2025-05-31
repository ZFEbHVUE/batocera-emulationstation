#include "guis/GuiFavoriteMusicSelector.h"
#include "components/MenuComponent.h"
#include "components/SwitchComponent.h"
#include "components/ButtonComponent.h"
#include "Window.h"
#include "Paths.h"
#include "FavoriteMusicManager.h"
#include "utils/FileSystemUtil.h"
#include "LocaleES.h"
#include <fstream>
#include <set>

GuiFavoriteMusicSelector::GuiFavoriteMusicSelector(Window* window) : GuiComponent(window)
{
    mMenu = new MenuComponent(window, _("FAVORITE SONGS"));
    
    loadMusic();
    
    // Boutons
    mMenu->addButton(_("SAVE"), _("SAVE"), [this] { save(); delete this; });
    mMenu->addButton(_("CANCEL"), _("CANCEL"), [this] { delete this; });
    
    addChild(mMenu);
    
    // Taille et position
    setSize(window->getSize() * 0.8f);
    setPosition((window->getSize() - getSize()) * 0.5f);
}

GuiFavoriteMusicSelector::~GuiFavoriteMusicSelector()
{
    delete mMenu;
}

void GuiFavoriteMusicSelector::openSelectFavoriteSongs(Window* window, bool, bool)
{
    window->pushGui(new GuiFavoriteMusicSelector(window));
}

void GuiFavoriteMusicSelector::loadMusic()
{
    std::function<void(const std::string&)> scan = [&](const std::string& path) {
        auto files = Utils::FileSystem::getDirContent(path, false);
        for (auto& file : files) {
            if (Utils::FileSystem::isDirectory(file)) {
                scan(file); // Récursif
            } else if (Utils::FileSystem::isAudio(file)) {
                mFiles.emplace_back(file, Utils::FileSystem::getFileName(file));
            }
        }
    };
    
    if (!Paths::getUserMusicPath().empty()) scan(Paths::getUserMusicPath());
    if (!Paths::getMusicPath().empty()) scan(Paths::getMusicPath());
    
    // Trier par nom
    std::sort(mFiles.begin(), mFiles.end(), 
              [](auto& a, auto& b) { return a.second < b.second; });
    
    std::set<std::string> favorites;
    auto favFile = FavoriteMusicManager::getFavoriteMusicFilePath();
    if (Utils::FileSystem::exists(favFile)) {
        auto favs = FavoriteMusicManager::loadFavoriteSongs(favFile);
        for (auto& fav : favs) favorites.insert(fav.first);
    }
    
    for (auto& file : mFiles) {
        auto sw = std::make_shared<SwitchComponent>(mWindow);
        sw->setState(favorites.count(file.first) > 0);
        mMenu->addWithLabel(file.second, sw);
        mSwitches.push_back(sw);
    }
}

void GuiFavoriteMusicSelector::save()
{
    auto favFile = FavoriteMusicManager::getFavoriteMusicFilePath();
   
    auto favPath = FavoriteMusicManager::getFavoriteMusicPath();
    if (!Utils::FileSystem::exists(favPath)) {
        Utils::FileSystem::createDirectory(favPath);
    }
    
    std::ofstream file(favFile);
    if (file.is_open()) {
        for (size_t i = 0; i < mFiles.size() && i < mSwitches.size(); i++) {
            if (mSwitches[i]->getState()) {
                file << mFiles[i].first << ";" << mFiles[i].second << "\n";
            }
        }
        file.close();
    }
    
    if (file.tellp() == 0) {
        Utils::FileSystem::removeFile(favFile);
    }
}
