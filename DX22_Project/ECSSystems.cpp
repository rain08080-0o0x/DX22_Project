#include "ECSSystems.h"
#include "ECSComponents.h"
#include "UIObject.h"

void ECSSystems::DrawUI(ECS::World& world)
{
    UIObject::Begin2D();

    world.Each<ECS::UIWidget>([](ECS::Entity, ECS::UIWidget& widget)
    {
        if (widget.visible && widget.ui)
        {
            widget.ui->Draw();
        }
    });

    world.Each<ECS::ScoreWidget>([](ECS::Entity, ECS::ScoreWidget& widget)
    {
        if (widget.visible && widget.score)
        {
            widget.score->Draw();
        }
    });

    world.Each<ECS::YukariComponent>([](ECS::Entity, ECS::YukariComponent& widget)
    {
        if (widget.visible && widget.yukari)
        {
            widget.yukari->Draw();
        }
    });

    UIObject::End2D();
}
