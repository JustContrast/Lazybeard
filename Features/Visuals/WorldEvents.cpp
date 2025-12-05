static std::string GetEventName(const std::string& actorClass)
{
    if (actorClass == "BP_SkellyFort_RitualSkullCloud_C") return "Fort Of The Damned";
    if (actorClass == "BP_LegendSkellyFort_SkullCloud_C") return "Fort Of Fortune";
    if (actorClass == "BP_GhostShips_Signal_Flameheart_NetProxy_C" ||
        actorClass == "BP_GhostShip_TornadoCloud_C") return "Flameheart/Ghost Fleet";
    if (actorClass == "BP_SkellyFort_SkullCloud_C") return "Skeleton Fort";
    if (actorClass == "BP_SkellyShip_ShipCloud_C") return "Skeleton Fleet";
    if (actorClass == "BP_AshenLord_SkullCloud_C") return "Ashen Winds";
    if (actorClass == "BP_ReaperTributeShipNetProxy_C" ||
        actorClass == "BP_ReapersTributeShipTemplate_C") return "Burning Blade";
    return "";
}


static void DrawEvent(const FVector& playerPos, const EntityEvents& entity)
{
    FVector actorPos = read<FVector>(entity.actor_root + _UnrealGlobals.RelativeLocation);
    Vector2 screenCoords;

    if (!WorldToScreen(actorPos, &screenCoords)) return;

    std::string eventName = GetEventName(entity.actor_class);
    if (eventName.empty()) return;

    double distance = playerPos.Distance(actorPos) / 100.0;
    std::string displayText = eventName + " (" + std::to_string(static_cast<int>(std::round(distance))) + "m)";

    DrawTextWithRoundedBackground(
        displayText.c_str(),
        ImVec2(screenCoords._x, screenCoords._y),
        ImColor(CFG::Colors::text_red, CFG::Colors::text_green, CFG::Colors::text_blue, 255),
        5
    );
}


void SDK::RenderLocalEvents()
{
    std::vector<EntityEvents> localCopy;
    {
        std::lock_guard<std::mutex> lock(events_mutex);
        localCopy = entityEvents; 
    }

    FVector playerPos = read<FVector>(Cache::RootComponent + _UnrealGlobals.RelativeLocation);

    for (const auto& entity : localCopy)
    {
        if (!entity.actor_pawn) continue;
        DrawEvent(playerPos, entity);
    }
}
