void SDK::RenderLocalWeather()
{
	std::vector<EntityWeather> localCopy;

	{
		std::lock_guard<std::mutex> lock(weather_mutex);
		localCopy = entityWeather; 
	}

	for (auto Entity : localCopy)
	{
		if (!Entity.actor_pawn) continue;

		FVector player_pos = read<FVector>(Cache::RootComponent + _UnrealGlobals.RelativeLocation);
		FVector actor_pos = read<FVector>(Entity.actor_root + _UnrealGlobals.RelativeLocation);

		Vector2 screen_coords;

		if (!WorldToScreen(actor_pos, &screen_coords)) continue;

		double distance = player_pos.Distance(actor_pos) / 100.0;
		std::string dis_text = "Storm (" + std::to_string((int)std::round(distance)) + "m)";

		DrawTextWithRoundedBackground(dis_text.c_str(),
			ImVec2(screen_coords._x, screen_coords._y),
			ImColor(CFG::Colors::text_red, CFG::Colors::text_green, CFG::Colors::text_blue, 255), 5);
	}
}
