#include "assembly.h"
#include "engine/log.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <engine/engine.h>
#include <engine/game.h>
#include <memory>
#include "registers.h"

class SDL_context {
public:
    SDL_context() {
        LOG_INFO("Initializing SDL");
        //SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            LOG_CRITICAL("Failed initializing SDL: %s", SDL_GetError());
            exit(1);
        }
        if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
            LOG_CRITICAL("Failed initializing SDL_image: %s", IMG_GetError());
            SDL_Quit();
            exit(1);
        }
        if (TTF_Init() < 0) {
            LOG_CRITICAL("Failed initializing SDL_ttf: %s", TTF_GetError());
            IMG_Quit();
            SDL_Quit();
            exit(1);
        }
        try {
            engine::init();
        } catch (base_exception& e) {
            LOG_CRITICAL("Failed initializing engine: %s", e.msg.c_str());
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            exit(1);
        }
        initialized = true;
    }

    ~SDL_context() {
        if (initialized) {
            initialized = false;
            LOG_INFO("Shutting down SDL libraries");
            engine::shutdown();
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
        }
    }
private:
    bool initialized = false;
};

std::unique_ptr<SDL_context> context;

int main(int argv, char* argc[]) {
    context = std::make_unique<SDL_context>();

    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);

    SDL_RWops* presets = SDL_RWFromFile("presets.json", "r");
    if (presets == nullptr) {
        LOG_CRITICAL("Failed opening presets file: %s", SDL_GetError());
        return 1;
    }
    std::size_t size = SDL_RWsize(presets);
    std::string str;
    str.resize(size);
    if (SDL_RWread(presets, &str[0], 1, size) != size) {
        LOG_CRITICAL("Failed reading presets file");
        SDL_RWclose(presets);
        return 1;
    }

    SDL_RWclose(presets);
    std::vector<ProcessorTemplate> templates;
    try {
        JsonObject obj = json::read_from_string(str);
        if (!obj.has_key_of_type<JsonList>("processors")) {
            LOG_CRITICAL("Failed parsing processor templates");
            return 1;
        }
        for (auto& o : obj.get<JsonList>("processors")) {
            if (JsonObject* obj = o.get<JsonObject>()) {
                templates.emplace_back();
                if (!templates.back().read_from_json(*obj)) {
                    LOG_CRITICAL("Failed parsing processor template");
                    templates.pop_back();
                    continue;
                }
            } else {
                LOG_CRITICAL("Failed parsing processor template");
            }
        }
        if (templates.size() == 0) {
            LOG_CRITICAL("No valid processor templates in file");
            return 1;
        }
    } catch (json_exception& e) {
        LOG_CRITICAL("Failed parsing presets.json: %s", e.msg.c_str());
        return 1;
    }

    LOG_DEBUG("%d processors loaded", templates.size());
    StateGame game {new GameState(std::move(templates)), 1920, 1080, "Text box!!!"};
    try {
        game.create();
        game.run();
    } catch (base_exception& e) {
        LOG_CRITICAL("%s", e.msg.c_str());
        return 1;
    }
    return 0;
}
