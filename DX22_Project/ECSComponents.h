#pragma once

#include <memory>
#include "Camera.h"
#include "Dice.h"
#include "ScoreLite.h"
#include "UIObject.h"
#include "Yukari.h"

namespace ECS
{
    struct UIWidget
    {
        std::unique_ptr<UIObject> ui;
        bool visible = true;
    };

    struct ScoreWidget
    {
        std::unique_ptr<ScoreLite> score;
        bool visible = true;
    };

    struct CameraComponent
    {
        std::unique_ptr<Camera> camera;
    };

    struct DiceComponent
    {
        std::unique_ptr<Dice> dice;
    };

    struct YukariComponent
    {
        std::unique_ptr<Yukari> yukari;
        bool visible = true;
    };
}
