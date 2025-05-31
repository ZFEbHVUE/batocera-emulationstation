#include "guis/GuiFavoriteMusicSelector.h"
#include "components/MenuComponent.h"
#include "components/SwitchComponent.h"
#include "Window.h"
#include "Paths.h"
#include "FavoriteMusicManager.h"
#include "utils/FileSystemUtil.h"
#include "LocaleES.h"
#include "renderers/Renderer.h"
#include <fstream>
#include <set>

GuiFavoriteMusicSelector::GuiFavoriteMusicSelector(Window* window) : GuiComponent(window)
{
    mMenu = new MenuComponent(window, _("FAVORITE SONGS"));
    
    // Scan récursif + ajout en une seule boucle
    std::function<void(const std::string&)> scan = [&](const std::string& path) {
        for (auto& file : Utils::FileSystem::getDirContent(path, false)) {
            if (Utils::FileSystem::isDirectory(file)) scan(file);
            else if (Utils::FileSystem::isAudio(file)) mFiles.emplace_back(file, Utils::FileSystem::getFileName(file));
        }
    };
    if (!Paths::getUserMusicPath().empty()) scan(Paths::getUserMusicPath());
    if (!Paths::getMusicPath().empty()) scan(Paths::getMusicPath());
    std::sort(mFiles.begin(), mFiles.end(), [](auto& a, auto& b) { return a.second < b.second; });
    
    // Charger favoris + créer liste en une boucle
    std::set<std::string> fav; 
    if (Utils::FileSystem::exists(FavoriteMusicManager::getFavoriteMusicFilePath()))
        for (auto& f : FavoriteMusicManager::loadFavoriteSongs(FavoriteMusicManager::getFavoriteMusicFilePath())) fav.insert(f.first);
    for (auto& file : mFiles) {
        auto sw = std::make_shared<SwitchComponent>(mWindow);
        sw->setState(fav.count(file.first));
        mMenu->addWithLabel(file.second, sw);
        mSwitches.push_back(sw);
    }
    
    addChild(mMenu);
    mMenu->setPosition((Renderer::getScreenWidth() - mMenu->getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu->getSize().y()) / 2);
}

GuiFavoriteMusicSelector::~GuiFavoriteMusicSelector() { 
    std::ofstream f(FavoriteMusicManager::getFavoriteMusicFilePath());
    for (size_t i = 0; i < mFiles.size() && i < mSwitches.size(); i++)
        if (mSwitches[i]->getState()) f << mFiles[i].first << ";" << mFiles[i].second << "\n";
    delete mMenu; 
}

void GuiFavoriteMusicSelector::openSelectFavoriteSongs(Window* window, bool, bool) { window->pushGui(new GuiFavoriteMusicSelector(window)); }

bool GuiFavoriteMusicSelector::input(InputConfig* config, Input input) {
    if (mMenu->input(config, input)) return true;
    if (input.value && config->isMappedTo("back", input)) { delete this; return true; }
    if (input.value && config->isMappedTo("b", input)) {
        int i = mMenu->getCursorIndex();
        if (i >= 0 && i < mSwitches.size()) { mSwitches[i]->setState(!mSwitches[i]->getState()); return true; }
    }
    return GuiComponent::input(config, input);
}

