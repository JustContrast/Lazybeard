#include "../../sdk.h"
#include "../../../globals.h"
#include "../../../CheatCFG.h"
#include <unordered_map>

using namespace CFG::Colors;

static const std::vector<std::pair<int, int>> kSkeleton = {
    {10, 80}, {80, 6}, {6, 5}, {5, 2},        // Head -> Neck -> Chest -> Stomach -> Pelvis
    {80, 82}, {82, 100}, {100, 101},          // Left arm
    {80, 74}, {74, 53}, {53, 72},             // Right arm
    {2, 115}, {115, 112}, {112, 122},         // Left leg
    {2, 110}, {110, 107}, {107, 109}          // Right leg
};

void DrawSkeleton(ImDrawList* drawList, const std::unordered_map<int, FVector>& bones, ImU32 color)
{
    for (const auto& [a, b] : kSkeleton)
    {
        Vector2 sa, sb;
        if (WorldToScreen(bones.at(a), &sa) && WorldToScreen(bones.at(b), &sb))
        {
            drawList->AddLine(ImVec2(sa._x, sa._y), ImVec2(sb._x, sb._y), color, 1.5f);
        }
    }
}

void SDK::RenderLocalPlayers()
{
    std::vector<EntityPlayer> localCopy;
    {
        std::lock_guard<std::mutex> lock(players_mutex);
        localCopy = entityPlayers;
    }

    for (const auto& entity : localCopy)
    {
        if (!entity.actor_pawn || entity.actor_pawn == Cache::AcknowledgedPawn)
            continue;

        FVector playerPos = read<FVector>(Cache::RootComponent + _UnrealGlobals.RelativeLocation);
        FVector actorPos  = read<FVector>(entity.actor_root + _UnrealGlobals.RelativeLocation);
        if (playerPos.x == 0 || playerPos.y == 0 || playerPos.z == 0) continue;
        if (actorPos.x == 0 || actorPos.y == 0 || actorPos.z == 0) continue;

        uintptr_t mesh = read<uintptr_t>(entity.actor_pawn + _UnrealGlobals.SkeletalMesh);
        if (!mesh) continue;

        uintptr_t healthComponent = read<uintptr_t>(entity.actor_pawn + 0x8a0);
        float healthPercent = 0.f;
        if (healthComponent)
        {
            float health = read<float>(healthComponent + 0xD4);
            float maxHealth = read<float>(healthComponent + 0xD0);
            if (maxHealth > 0.f) healthPercent = (health / maxHealth) * 100.f;
        }

        double distance = playerPos.Distance(actorPos) / 100.0;
        if (distance > 250) continue;

        auto drawList = ImGui::GetBackgroundDrawList();

        std::unordered_map<int, FVector> boneCache;
        for (const auto& [a, b] : kSkeleton)
        {
            if (!boneCache.count(a)) boneCache[a] = GetSocketWorldPos(mesh, a);
            if (!boneCache.count(b)) boneCache[b] = GetSocketWorldPos(mesh, b);
        }
        if (!boneCache.count(10)) boneCache[10] = GetSocketWorldPos(mesh, 10);
        if (!boneCache.count(0))  boneCache[0]  = GetSocketWorldPos(mesh, 0);

        if (CFG::Visuals::player_skeleton)
            DrawSkeleton(drawList, boneCache, IM_COL32(255, 255, 255, 255));

        Vector2 headScreen, rootScreen;
        if (!WorldToScreen(boneCache[10], &headScreen) || !WorldToScreen(boneCache[0], &rootScreen))
            continue;

        int hi = rootScreen._y - headScreen._y;
        int wi = hi / 2;
        int boxLeft = headScreen._x - wi / 2;
        int boxTop  = headScreen._y;

        if (CFG::Visuals::player_box)
            draw_cornered_box2(
                boxLeft, boxTop, wi, hi, 1,
                ImColor(box_red, box_green, box_blue, 255),
                ImColor(0, 0, 0, 255)
            );

        if (CFG::Visuals::player_lines)
            DrawLine(
                ImVec2(ScreenWidth / 2, ScreenHeight),
                ImVec2(headScreen._x, rootScreen._y),
                ImColor(lines_red, lines_green, lines_blue, 255),
                1.f
            );

        if (CFG::Visuals::player_health && healthPercent > 0.f)
        {
            float width2 = std::clamp(wi / 10.f, 2.f, 3.f);
            HealthBar(boxLeft - width2 - 4, boxTop, width2, hi, healthPercent, true);
        }

        // player info
        std::string infoText;
        if (CFG::Visuals::player_name)
        {
            FString playerName = read<FString>(entity.PlayerState + 0x3A8);
            infoText += playerName.ToString() + " (" + std::to_string(static_cast<int>(distance)) + "m)";
        }

        if (CFG::Visuals::current_weapon)
        {
            if (uintptr_t wieldedComp = read<uintptr_t>(Cache::AcknowledgedPawn + 0x870))
            {
                if (uintptr_t currentWeapon = read<uintptr_t>(wieldedComp + 0x2e0))
                {
                    std::string weaponName = GName(read<int>(currentWeapon + _UnrealGlobals.ActorId));
                    infoText += weaponName;
                }
            }
        }

        // directional arrows
        if (CFG::Visuals::arrows)
        {
            const float ARROW_SIZE = 15.5f;
            float dynamicFOV = (dynamicvariables::curfov - 80) + CFG::Aimbot::fov_circle_size_weapon;
            const float ATTACH_DIST = 8.0f;

            ImVec2 center = ImVec2(ScreenWidth / 2, ScreenHeight / 2);
            ImVec2 dir = ImVec2(headScreen._x - center.x, headScreen._y - center.y);

            float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
            dir.x /= len; dir.y /= len;

            ImVec2 arrowPos = ImVec2(center.x + dir.x * (dynamicFOV + ATTACH_DIST),
                                     center.y + dir.y * (dynamicFOV + ATTACH_DIST));

            float angle = atan2f(dir.y, dir.x);

            drawList->AddTriangleFilled(
                ImVec2(arrowPos.x + cosf(angle) * ARROW_SIZE, arrowPos.y + sinf(angle) * ARROW_SIZE),
                ImVec2(arrowPos.x + cosf(angle + 2.35619f) * ARROW_SIZE, arrowPos.y + sinf(angle + 2.35619f) * ARROW_SIZE),
                ImVec2(arrowPos.x + cosf(angle - 2.35619f) * ARROW_SIZE, arrowPos.y + sinf(angle - 2.35619f) * ARROW_SIZE),
                ImColor(255, 0, 0, 255)
            );
        }

        if (!infoText.empty())
        {
            drawList->AddText(
                ImVec2(rootScreen._x, rootScreen._y + 5),
                ImColor(CFG::Colors::text_red, CFG::Colors::text_green, CFG::Colors::text_blue, 255),
                infoText.c_str()
            );
        }
    }
}
