//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		yet another ouendan clone, because why not
//
// $NoKeywords: $osu
//===============================================================================//

#include "OsuDatabase.h"
#include "Osu.h"

#include "Engine.h"
#include "Environment.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "ConsoleBox.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "SoundEngine.h"
#include "Console.h"
#include "ConVar.h"
#include "SteamworksInterface.h"
#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "RenderTarget.h"
#include "Shader.h"

#include "CWindowManager.h"
//#include "DebugMonitor.h"

#include "Osu2.h"
#include "OsuVR.h"
#include "OsuMultiplayer.h"
#include "OsuMainMenu.h"
#include "OsuOptionsMenu.h"
#include "OsuSongBrowser2.h"
#include "OsuModSelector.h"
#include "OsuRankingScreen.h"
#include "OsuUserStatsScreen.h"
#include "OsuKeyBindings.h"
#include "OsuUpdateHandler.h"
#include "OsuNotificationOverlay.h"
#include "OsuTooltipOverlay.h"
#include "OsuGameRules.h"
#include "OsuPauseMenu.h"
#include "OsuScore.h"
#include "OsuSkin.h"
#include "OsuIcons.h"
#include "OsuHUD.h"
#include "OsuVRTutorial.h"
#include "OsuChangelog.h"
#include "OsuEditor.h"
#include "OsuRichPresence.h"
#include "OsuSteamWorkshop.h"
#include "OsuModFPoSu.h"

#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuBeatmapStandard.h"
#include "OsuBeatmapMania.h"

#include "OsuHitObject.h"

#include "OsuUIVolumeSlider.h"

// release configuration
bool Osu::autoUpdater = false;
ConVar osu_version("osu_version", 30.0f);
#ifdef MCENGINE_FEATURE_OPENVR
ConVar osu_release_stream("osu_release_stream", "vr");
#else
ConVar osu_release_stream("osu_release_stream", "desktop");
#endif
ConVar osu_debug("osu_debug", false);

ConVar osu_vr("osu_vr", false);
ConVar osu_vr_tutorial("osu_vr_tutorial", true);

ConVar osu_disable_mousebuttons("osu_disable_mousebuttons", false);
ConVar osu_disable_mousewheel("osu_disable_mousewheel", false);
ConVar osu_confine_cursor_windowed("osu_confine_cursor_windowed", false);
ConVar osu_confine_cursor_fullscreen("osu_confine_cursor_fullscreen", true);

ConVar osu_skin("osu_skin", ""); // set dynamically below in the constructor
ConVar osu_skin_is_from_workshop("osu_skin_is_from_workshop", false, "determines whether osu_skin contains a relative folder name, or a full absolute path (for workshop skins)");
ConVar osu_skin_workshop_title("osu_skin_workshop_title", "", "holds the title/name of the currently selected workshop skin, because osu_skin is already used up for the absolute path then");
ConVar osu_skin_workshop_id("osu_skin_workshop_id", "0", "holds the id of the currently selected workshop skin");
ConVar osu_skin_reload("osu_skin_reload");

ConVar osu_volume_master("osu_volume_master", 1.0f);
ConVar osu_volume_music("osu_volume_music", 0.4f);
ConVar osu_volume_change_interval("osu_volume_change_interval", 0.05f);

ConVar osu_speed_override("osu_speed_override", -1.0f);
ConVar osu_pitch_override("osu_pitch_override", -1.0f);

ConVar osu_pause_on_focus_loss("osu_pause_on_focus_loss", true);
ConVar osu_quick_retry_delay("osu_quick_retry_delay", 0.27f);

ConVar osu_mods("osu_mods", "");
ConVar osu_mod_fadingcursor("osu_mod_fadingcursor", false);
ConVar osu_mod_fadingcursor_combo("osu_mod_fadingcursor_combo", 50.0f);
ConVar osu_mod_endless("osu_mod_endless", false);

ConVar osu_notification("osu_notification");

ConVar osu_letterboxing("osu_letterboxing", true);
ConVar osu_letterboxing_offset_x("osu_letterboxing_offset_x", 0.0f);
ConVar osu_letterboxing_offset_y("osu_letterboxing_offset_y", 0.0f);
ConVar osu_resolution("osu_resolution", "1280x720");
ConVar osu_resolution_enabled("osu_resolution_enabled", false);
ConVar osu_resolution_keep_aspect_ratio("osu_resolution_keep_aspect_ratio", false);
ConVar osu_force_legacy_slider_renderer("osu_force_legacy_slider_renderer", false, "on some older machines, this may be faster than vertexbuffers");

ConVar osu_draw_fps("osu_draw_fps", true);
ConVar osu_hide_cursor_during_gameplay("osu_hide_cursor_during_gameplay", false);

ConVar *Osu::version = &osu_version;
ConVar *Osu::debug = &osu_debug;
Vector2 Osu::g_vInternalResolution;
Vector2 Osu::osuBaseResolution = Vector2(640.0f, 480.0f);

Osu::Osu(Osu2 *osu2, int instanceID)
{
	srand(time(NULL));

	m_osu2 = osu2;
	m_iInstanceID = instanceID;

	// convar refs
	m_osu_folder_ref = convar->getConVarByName("osu_folder");
	m_osu_folder_sub_skins_ref = convar->getConVarByName("osu_folder_sub_skins");
	m_osu_draw_hud_ref = convar->getConVarByName("osu_draw_hud");
	m_osu_draw_scoreboard = convar->getConVarByName("osu_draw_scoreboard");
	m_osu_mod_fps_ref = convar->getConVarByName("osu_mod_fps");
	m_osu_mod_minimize_ref = convar->getConVarByName("osu_mod_minimize");
	m_osu_mod_wobble_ref = convar->getConVarByName("osu_mod_wobble");
	m_osu_mod_wobble2_ref = convar->getConVarByName("osu_mod_wobble2");
	m_osu_playfield_rotation = convar->getConVarByName("osu_playfield_rotation");
	m_osu_playfield_stretch_x = convar->getConVarByName("osu_playfield_stretch_x");
	m_osu_playfield_stretch_y = convar->getConVarByName("osu_playfield_stretch_y");
	m_osu_draw_cursor_trail_ref = convar->getConVarByName("osu_draw_cursor_trail");
	m_osu_volume_effects_ref = convar->getConVarByName("osu_volume_effects");
	m_osu_mod_mafham_ref = convar->getConVarByName("osu_mod_mafham");
	m_osu_mod_fposu_ref = convar->getConVarByName("osu_mod_fposu");
	m_snd_change_check_interval_ref = convar->getConVarByName("snd_change_check_interval");

	// experimental mods list
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_wobble"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_arwobble"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_timewarp"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_artimewarp"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_minimize"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_fadingcursor"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_fps"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_jigsaw1"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_jigsaw2"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_fullalternate"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_random"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_no50s"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_no100s"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_ming3012"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_millhioref"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_mafham"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_playfield_mirror_horizontal"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_playfield_mirror_vertical"));

	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_wobble2"));
	m_experimentalMods.push_back(convar->getConVarByName("osu_mod_shirone"));

	// engine settings/overrides
	engine->getSound()->setOnOutputDeviceChange([this] {onAudioOutputDeviceChange();});
	openvr->setDrawCallback( fastdelegate::MakeDelegate(this, &Osu::drawVR) );
	if (openvr->isReady()) // automatically enable VR mode if it was compiled with OpenVR support and is available
		osu_vr.setValue(1.0f);

	env->setWindowTitle("McOsu");
	env->setCursorVisible(false);

	engine->getConsoleBox()->setRequireShiftToActivate(true);
	engine->getSound()->setVolume(osu_volume_master.getFloat());
	if (m_iInstanceID < 2)
		engine->getMouse()->addListener(this);

	convar->getConVarByName("name")->setValue("Guest");
	convar->getConVarByName("console_overlay")->setValue(0.0f);
	convar->getConVarByName("vsync")->setValue(0.0f);
	convar->getConVarByName("fps_max")->setValue(420.0f);
	convar->getConVarByName("fps_max")->setDefaultFloat(420.0f);

	m_snd_change_check_interval_ref->setDefaultFloat(0.5f);
	m_snd_change_check_interval_ref->setValue(m_snd_change_check_interval_ref->getDefaultFloat());

	osu_resolution.setValue(UString::format("%ix%i", engine->getScreenWidth(), engine->getScreenHeight()));

	// init steam rich presence localization
	steam->setRichPresence("steam_display", "#Status");
	steam->setRichPresence("status", "...");

#ifdef MCENGINE_FEATURE_BASS_WASAPI

	// since we use the newer bass/fx dlls for wasapi builds anyway (which have different time handling)
	convar->getConVarByName("osu_universal_offset_hardcoded")->setValue(-25.0f);

#endif

	// OS specific engine settings/overrides
	if (env->getOS() == Environment::OS::OS_HORIZON)
	{
		convar->getConVarByName("fps_max")->setValue(60.0f);
		convar->getConVarByName("ui_scrollview_resistance")->setValue(25.0f);
		convar->getConVarByName("osu_scores_legacy_enabled")->setValue(0.0f); // would collide
		convar->getConVarByName("osu_mod_mafham_render_livesize")->setValue(7.0f);
		convar->getConVarByName("osu_mod_mafham_render_chunksize")->setValue(12.0f);
		convar->getConVarByName("osu_volume_music")->setValue(0.3f);
		convar->getConVarByName("osu_universal_offset_hardcoded")->setValue(-45.0f);
		convar->getConVarByName("osu_key_quick_retry")->setValue(15.0f);	// L, SDL_SCANCODE_L
		convar->getConVarByName("osu_key_seek_time")->setValue(21.0f);		// R, SDL_SCANCODE_R
		convar->getConVarByName("osu_key_decrease_local_offset")->setValue(29.0f); // ZL, SDL_SCANCODE_Z
		convar->getConVarByName("osu_key_increase_local_offset")->setValue(25.0f); // ZR, SDL_SCANCODE_V
		convar->getConVarByName("osu_key_left_click")->setValue(0.0f);		// (disabled)
		convar->getConVarByName("osu_key_right_click")->setValue(0.0f);	// (disabled)
		convar->getConVarByName("name")->setValue(env->getUsername());
	}

	// VR specific settings
	if (isInVRMode())
	{
		osu_skin.setValue("defaultvr");
		convar->getConVarByName("osu_drain_enabled")->setValue(true);
		convar->getConVarByName("osu_draw_hpbar")->setValue(true);
		env->setWindowResizable(true);
	}
	else
	{
		osu_skin.setValue("default");
		env->setWindowResizable(false);
	}

	// generate default osu! appdata user path
	UString userDataPath = env->getUserDataPath();
	if (userDataPath.length() > 1)
	{
		UString defaultOsuFolder = userDataPath;
		defaultOsuFolder.append(env->getOS() == Environment::OS::OS_WINDOWS ? "\\osu!\\" : "/osu!/");
		m_osu_folder_ref->setValue(defaultOsuFolder);
	}

	// convar callbacks
	osu_skin.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSkinChange) );
	osu_skin_reload.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSkinReload) );

	osu_volume_master.setCallback( fastdelegate::MakeDelegate(this, &Osu::onMasterVolumeChange) );
	osu_volume_music.setCallback( fastdelegate::MakeDelegate(this, &Osu::onMusicVolumeChange) );
	osu_speed_override.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSpeedChange) );
	osu_pitch_override.setCallback( fastdelegate::MakeDelegate(this, &Osu::onPitchChange) );

	m_osu_playfield_rotation->setCallback( fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange) );
	m_osu_playfield_stretch_x->setCallback( fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange) );
	m_osu_playfield_stretch_y->setCallback( fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange) );

	osu_mods.setCallback( fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate) );

	osu_resolution.setCallback( fastdelegate::MakeDelegate(this, &Osu::onInternalResolutionChanged) );
	osu_letterboxing.setCallback( fastdelegate::MakeDelegate(this, &Osu::onLetterboxingChange) );
	osu_letterboxing_offset_x.setCallback( fastdelegate::MakeDelegate(this, &Osu::onLetterboxingOffsetChange) );
	osu_letterboxing_offset_y.setCallback( fastdelegate::MakeDelegate(this, &Osu::onLetterboxingOffsetChange) );

	osu_confine_cursor_windowed.setCallback( fastdelegate::MakeDelegate(this, &Osu::onConfineCursorWindowedChange) );
	osu_confine_cursor_fullscreen.setCallback( fastdelegate::MakeDelegate(this, &Osu::onConfineCursorFullscreenChange) );

	convar->getConVarByName("osu_playfield_mirror_horizontal")->setCallback( fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate) ); // force a mod update on OsuBeatmap if changed
	convar->getConVarByName("osu_playfield_mirror_vertical")->setCallback( fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate) ); // force a mod update on OsuBeatmap if changed

	osu_notification.setCallback( fastdelegate::MakeDelegate(this, &Osu::onNotification) );

  	// vars
	m_skin = NULL;
	m_songBrowser2 = NULL;
	m_modSelector = NULL;
	m_updateHandler = NULL;
	m_multiplayer = NULL;

	m_bF1 = false;
	m_bUIToggleCheck = false;
	m_bScoreboardToggleCheck = false;
	m_bTab = false;
	m_bEscape = false;
	m_bKeyboardKey1Down = false;
	m_bKeyboardKey2Down = false;
	m_bMouseKey1Down = false;
	m_bMouseKey2Down = false;
	m_bSkipScheduled = false;
	m_bQuickRetryDown = false;
	m_fQuickRetryTime = 0.0f;
	m_bSeeking = false;
	m_bSeekKey = false;
	m_fQuickSaveTime = 0.0f;

	m_bToggleModSelectionScheduled = false;
	m_bToggleSongBrowserScheduled = false;
	m_bToggleOptionsMenuScheduled = false;
	m_bOptionsMenuFullscreen = true;
	m_bToggleRankingScreenScheduled = false;
	m_bToggleUserStatsScreenScheduled = false;
	m_bToggleVRTutorialScheduled = false;
	m_bToggleChangelogScheduled = false;
	m_bToggleEditorScheduled = false;

	m_bModAuto = false;
	m_bModAutopilot = false;
	m_bModRelax = false;
	m_bModSpunout = false;
	m_bModTarget = false;
	m_bModScorev2 = false;
	m_bModDT = false;
	m_bModNC = false;
	m_bModNF = false;
	m_bModHT = false;
	m_bModDC = false;
	m_bModHD = false;
	m_bModHR = false;
	m_bModEZ = false;
	m_bModSD = false;
	m_bModSS = false;
	m_bModNM = false;

	m_bShouldCursorBeVisible = false;

	m_gamemode = GAMEMODE::STD;
	m_bScheduleEndlessModNextBeatmap = false;
	m_iMultiplayerClientNumEscPresses = 0;
	m_bIsInVRDraw = false;
	m_bWasBossKeyPaused = false;
	m_bSkinLoadScheduled = false;
	m_bSkinLoadWasReload = false;
	m_skinScheduledToLoad = NULL;

	// debug
	m_windowManager = new CWindowManager();
	/*
	DebugMonitor *dm = new DebugMonitor();
	m_windowManager->addWindow(dm);
	dm->open();
	*/

	// renderer
	g_vInternalResolution = engine->getScreenSize();

	m_backBuffer = engine->getResourceManager()->createRenderTarget(0, 0, getScreenWidth(), getScreenHeight());
	m_playfieldBuffer = engine->getResourceManager()->createRenderTarget(0, 0, 64, 64);
	m_sliderFrameBuffer = engine->getResourceManager()->createRenderTarget(0, 0, getScreenWidth(), getScreenHeight());
	m_frameBuffer = engine->getResourceManager()->createRenderTarget(0, 0, 64, 64);
	m_frameBuffer2 = engine->getResourceManager()->createRenderTarget(0, 0, 64, 64);

	// load a few select subsystems very early
	m_notificationOverlay = new OsuNotificationOverlay(this);
	m_score = new OsuScore(this);
	m_updateHandler = new OsuUpdateHandler();

	// exec the main config file (this must be right here!)
	if (m_iInstanceID < 2)
		Console::execConfigFile(isInVRMode() ? "osuvr" : "osu");

	// update mod settings
	updateMods();

	// load global resources
	m_titleFont = engine->getResourceManager()->loadFont("SourceSansPro-Semibold.otf", "FONT_OSU_TITLE", 60.0f); // was 40
	m_subTitleFont = engine->getResourceManager()->loadFont("SourceSansPro-Semibold.otf", "FONT_OSU_SUBTITLE", 21.0f);
	m_songBrowserFont = engine->getResourceManager()->loadFont("SourceSansPro-Regular.otf", "FONT_OSU_SONGBROWSER", 35.0f);
	m_songBrowserFontBold = engine->getResourceManager()->loadFont("SourceSansPro-Bold.otf", "FONT_OSU_SONGBROWSER_BOLD", 30.0f);
	m_fontIcons = engine->getResourceManager()->loadFont("fontawesome-webfont.ttf", "FONT_OSU_ICONS", OsuIcons::icons, 26.0f);

	float averageIconHeight = 0.0f;
	for (int i=0; i<OsuIcons::icons.size(); i++)
	{
		UString iconString; iconString.insert(0, OsuIcons::icons[i]);
		const float height = m_fontIcons->getStringHeight(iconString);
		if (height > averageIconHeight)
			averageIconHeight = height;
	}
	m_fontIcons->setHeight(averageIconHeight);

	// load skin
	{
		UString skinFolder = m_osu_folder_ref->getString();
		skinFolder.append(m_osu_folder_sub_skins_ref->getString());
		skinFolder.append(osu_skin.getString());
		skinFolder.append("/");
		if (m_skin == NULL) // the skin may already be loaded by Console::execConfigFile() above
			onSkinChange("", osu_skin.getString());

		// enable async skin loading for user-action skin changes (but not during startup)
		OsuSkin::m_osu_skin_async->setValue(1.0f);
	}

	// load subsystems, add them to the screens array
	m_tooltipOverlay = new OsuTooltipOverlay(this);
	m_vr = new OsuVR(this);
	m_multiplayer = new OsuMultiplayer(this);
	m_mainMenu = new OsuMainMenu(this);
	m_optionsMenu = new OsuOptionsMenu(this);
	m_songBrowser2 = new OsuSongBrowser2(this);
	m_modSelector = new OsuModSelector(this);
	m_rankingScreen = new OsuRankingScreen(this);
	m_userStatsScreen = new OsuUserStatsScreen(this);
	m_pauseMenu = new OsuPauseMenu(this);
	m_hud = new OsuHUD(this);
	m_vrTutorial = new OsuVRTutorial(this);
	m_changelog = new OsuChangelog(this);
	m_editor = new OsuEditor(this);
	m_steamWorkshop = new OsuSteamWorkshop(this);
	m_fposu = new OsuModFPoSu(this);

	// the order in this vector will define in which order events are handled/consumed
	m_screens.push_back(m_notificationOverlay);
	m_screens.push_back(m_optionsMenu);
	m_screens.push_back(m_userStatsScreen);
	m_screens.push_back(m_rankingScreen);
	m_screens.push_back(m_modSelector);
	m_screens.push_back(m_pauseMenu);
	m_screens.push_back(m_hud);
	m_screens.push_back(m_songBrowser2);
	m_screens.push_back(m_vrTutorial);
	m_screens.push_back(m_changelog);
	m_screens.push_back(m_editor);
	m_screens.push_back(m_mainMenu);
	m_screens.push_back(m_tooltipOverlay);

	// make primary screen visible
	//m_optionsMenu->setVisible(true);
	//m_modSelector->setVisible(true);
	//m_songBrowser2->setVisible(true);
	//m_pauseMenu->setVisible(true);
	//m_rankingScreen->setVisible(true);
	//m_changelog->setVisible(true);
	//m_editor->setVisible(true);
	//m_userStatsScreen->setVisible(true);

	if (isInVRMode() && osu_vr_tutorial.getBool())
		m_vrTutorial->setVisible(true);
	else
		m_mainMenu->setVisible(true);

	m_updateHandler->checkForUpdates();

	/*
	// DEBUG: immediately start diff of a beatmap
	UString debugFolder = "C:/Program Files (x86)/osu!/Songs/537048 Erehamonika remixed by kors k - Der Wald (Kors K Remix)/";
	UString debugDiffFileName = "Erehamonika remixed by kors k - Der Wald (Kors K Remix) (Rucker) [Maze].osu";
	OsuBeatmap *debugBeatmap = new OsuBeatmapStandard(this);
	UString beatmapPath = debugFolder;
	beatmapPath.append(debugDiffFileName);
	OsuBeatmapDifficulty *debugDiff = new OsuBeatmapDifficulty(this, beatmapPath, debugFolder);
	if (!debugDiff->loadMetadataRaw())
		engine->showMessageError("OsuBeatmapDifficulty", "Couldn't debugDiff->loadMetadataRaw()!");
	else
	{
		std::vector<OsuBeatmapDifficulty*> diffs;
		diffs.push_back(debugDiff);
		debugBeatmap->setDifficulties(diffs);

		debugBeatmap->selectDifficulty(debugDiff);
		m_songBrowser2->onDifficultySelected(debugBeatmap, debugDiff, true);
		//convar->getConVarByName("osu_volume_master")->setValue(1.0f);

		// this will leak memory (one OsuBeatmap object and one OsuBeatmapDifficulty object), but who cares (since debug only)
	}
	*/

	// DEBUG: immediately start diff of a beatmap
	/*
	UString debugFolder = "c:/Program Files (x86)/osu!/Songs/407186 S3RL feat Krystal - R4V3 B0Y/";
	UString debugDiffFileName = "S3RL feat Krystal - R4V3 B0Y (Draftnell) [BANGKE's 4K Normal].osu";
	OsuBeatmap *debugBeatmap = new OsuBeatmapMania(this);
	UString beatmapPath = debugFolder;
	beatmapPath.append(debugDiffFileName);
	OsuBeatmapDifficulty *debugDiff = new OsuBeatmapDifficulty(this, beatmapPath, debugFolder);
	if (!debugDiff->loadMetadataRaw())
		engine->showMessageError("OsuBeatmapDifficulty", "Couldn't debugDiff->loadMetadataRaw()!");
	else
	{
		std::vector<OsuBeatmapDifficulty*> diffs;
		diffs.push_back(debugDiff);
		debugBeatmap->setDifficulties(diffs);

		debugBeatmap->selectDifficulty(debugDiff);
		m_songBrowser2->onDifficultySelected(debugBeatmap, debugDiff, true);
		//convar->getConVarByName("osu_volume_master")->setValue(1.0f);

		// this will leak memory (one OsuBeatmap object and one OsuBeatmapDifficulty object), but who cares (since debug only)
	}
	*/



	// memory/performance optimization; if osu_mod_mafham is not enabled, reduce the two rendertarget sizes to 64x64, same for fposu
	m_osu_mod_mafham_ref->setCallback( fastdelegate::MakeDelegate(this, &Osu::onModMafhamChange) );
	m_osu_mod_fposu_ref->setCallback( fastdelegate::MakeDelegate(this, &Osu::onModFPoSuChange) );
}

Osu::~Osu()
{
	// "leak" OsuUpdateHandler object, but not relevant since shutdown:
	// this is the only way of handling instant user shutdown requests properly, there is no solution for active working threads besides letting the OS kill them when the main threads exits.
	// we must not delete the update handler object, because the thread is potentially still accessing members during shutdown
	m_updateHandler->stop(); // tell it to stop at the next cancellation point, depending on the OS/runtime and engine shutdown time it may get killed before that

	SAFE_DELETE(m_windowManager);

	for (int i=0; i<m_screens.size(); i++)
	{
		debugLog("%i\n", i);
		SAFE_DELETE(m_screens[i]);
	}

	SAFE_DELETE(m_steamWorkshop);
	SAFE_DELETE(m_fposu);

	SAFE_DELETE(m_score);
	SAFE_DELETE(m_vr);
	SAFE_DELETE(m_multiplayer);
	SAFE_DELETE(m_skin);
}

void Osu::draw(Graphics *g)
{
	if (m_skin == NULL) // sanity check
	{
		g->setColor(0xffff0000);
		g->fillRect(0, 0, getScreenWidth(), getScreenHeight());
		return;
	}

	// if we are not using the native window resolution, or in vr mode, or playing on a nintendo switch, or multiple instances are active, draw into the buffer
	const bool isBufferedDraw = osu_resolution_enabled.getBool() || isInVRMode() || env->getOS() == Environment::OS::OS_HORIZON || m_iInstanceID > 0;

	if (isBufferedDraw)
		m_backBuffer->enable();

	// draw everything in the correct order
	if (isInPlayMode()) // if we are playing a beatmap
	{
		const bool isBufferedPlayfieldDraw = m_osu_mod_fposu_ref->getBool();

		if (isBufferedPlayfieldDraw)
			m_playfieldBuffer->enable();

		getSelectedBeatmap()->draw(g);

		if (!m_osu_mod_fposu_ref->getBool())
			m_hud->draw(g);

		// quick retry fadeout overlay
		if (m_fQuickRetryTime != 0.0f && m_bQuickRetryDown)
		{
			float alphaPercent = 1.0f - (m_fQuickRetryTime - engine->getTime())/osu_quick_retry_delay.getFloat();
			if (engine->getTime() > m_fQuickRetryTime)
				alphaPercent = 1.0f;

			g->setColor(COLOR((int)(255*alphaPercent), 0, 0, 0));
			g->fillRect(0, 0, getScreenWidth(), getScreenHeight());
		}

		// special cursor handling (fading cursor + invisible cursor mods + draw order etc.)
		const bool isAuto = (m_bModAuto || m_bModAutopilot);
		const bool isFPoSu = (m_osu_mod_fposu_ref->getBool());
		const bool allowDoubleCursor = (env->getOS() == Environment::OS::OS_HORIZON || isFPoSu);
		const bool allowDrawCursor = (!osu_hide_cursor_during_gameplay.getBool() || getSelectedBeatmap()->isPaused());
		float fadingCursorAlpha = 1.0f - clamp<float>((float)m_score->getCombo()/osu_mod_fadingcursor_combo.getFloat(), 0.0f, 1.0f);
		if (m_pauseMenu->isVisible() || getSelectedBeatmap()->isContinueScheduled())
			fadingCursorAlpha = 1.0f;

		OsuBeatmapStandard *beatmapStd = dynamic_cast<OsuBeatmapStandard*>(getSelectedBeatmap());

		// draw auto cursor
		if (isAuto && allowDrawCursor && !isFPoSu && beatmapStd != NULL && !beatmapStd->isLoading())
			m_hud->drawCursor(g, m_osu_mod_fps_ref->getBool() ? OsuGameRules::getPlayfieldCenter(this) : beatmapStd->getCursorPos(), osu_mod_fadingcursor.getBool() ? fadingCursorAlpha : 1.0f);

		m_pauseMenu->draw(g);
		m_modSelector->draw(g);
		m_optionsMenu->draw(g);

		if (osu_draw_fps.getBool() && !isFPoSu)
			m_hud->drawFps(g);

		m_hud->drawVolumeChange(g);

		m_windowManager->draw(g);

		if (isBufferedPlayfieldDraw)
			m_playfieldBuffer->disable();

		if (isFPoSu)
		{
			m_fposu->draw(g);
			m_hud->draw(g);
			m_hud->drawFps(g);
		}

		// draw player cursor
		if ((!isAuto || allowDoubleCursor) && allowDrawCursor && (!isInVRMode() || (m_vr->isVirtualCursorOnScreen() || engine->hasFocus())))
		{
			Vector2 cursorPos = (beatmapStd != NULL && !isAuto) ? beatmapStd->getCursorPos() : engine->getMouse()->getPos();

			if (isFPoSu)
				cursorPos = getScreenSize() / 2.0f;

			m_hud->drawCursor(g, cursorPos, (osu_mod_fadingcursor.getBool() && !isAuto) ? fadingCursorAlpha : 1.0f, isAuto);
		}

		// draw projected VR cursors for spectators
		if (isInVRMode() && isInPlayMode() && !getSelectedBeatmap()->isPaused() && beatmapStd != NULL)
		{
			m_hud->drawCursorSpectator1(g, beatmapStd->osuCoords2RawPixels(m_vr->getCursorPos1() + Vector2(OsuGameRules::OSU_COORD_WIDTH/2, OsuGameRules::OSU_COORD_HEIGHT/2)), 1.0f);
			m_hud->drawCursorSpectator2(g, beatmapStd->osuCoords2RawPixels(m_vr->getCursorPos2() + Vector2(OsuGameRules::OSU_COORD_WIDTH/2, OsuGameRules::OSU_COORD_HEIGHT/2)), 1.0f);
		}
	}
	else // if we are not playing
	{
		if (m_songBrowser2 != NULL)
			m_songBrowser2->draw(g);

		m_modSelector->draw(g);
		m_mainMenu->draw(g);
		m_vrTutorial->draw(g);
		m_changelog->draw(g);
		m_editor->draw(g);
		m_userStatsScreen->draw(g);
		m_rankingScreen->draw(g);
		m_optionsMenu->draw(g);

		if (isInMultiplayer())
			m_hud->drawScoreBoardMP(g);

		if (osu_draw_fps.getBool())
			m_hud->drawFps(g);

		m_hud->drawVolumeChange(g);

		m_windowManager->draw(g);

		if (!isInVRMode() || (m_vr->isVirtualCursorOnScreen() || engine->hasFocus()))
			m_hud->drawCursor(g, engine->getMouse()->getPos());
	}

	// TODO: TEMP:
	/*
	if (m_multiplayer->isInMultiplayer() && m_multiplayer->isServer())
	{
		for (int i=0; i<m_multiplayer->getServerPlayers()->size(); i++)
		{
			OsuMultiplayer::PLAYER *ply = &(*m_multiplayer->getServerPlayers())[i];
			m_hud->drawCursor(g, ply->input.cursorPos, 0.5f);
		}
	}
	*/

	m_tooltipOverlay->draw(g);
	m_notificationOverlay->draw(g);

	// loading spinner for some async tasks
	if ((m_bSkinLoadScheduled && m_skin != m_skinScheduledToLoad) || m_optionsMenu->isWorkshopLoading() || m_steamWorkshop->isUploading())
	{
		m_hud->drawLoadingSmall(g);
	}

	// if we are not using the native window resolution;
	// we must also do this if we are in VR mode, since we only draw once and the buffer is used to draw the virtual screen later. otherwise we wouldn't see anything on the desktop window
	if (isBufferedDraw)
	{
		// draw a scaled version from the buffer to the screen
		m_backBuffer->disable();

		// TODO: move this shit to Osu2
		Vector2 offset = Vector2(engine->getGraphics()->getResolution().x/2 - g_vInternalResolution.x/2, engine->getGraphics()->getResolution().y/2 - g_vInternalResolution.y/2);
		if (m_iInstanceID > 0)
		{
			const int numHorizontalInstances = 2 + (m_osu2->getNumInstances() > 4 ? 1 : 0);
			const int numVerticalInstances = 1 + (m_osu2->getNumInstances() > 2 ? 1 : 0) + (m_osu2->getNumInstances() > 8 ? 1 : 0);

			float emptySpaceX = engine->getGraphics()->getResolution().x - numHorizontalInstances*g_vInternalResolution.x;
			float emptySpaceY = engine->getGraphics()->getResolution().y - numVerticalInstances*g_vInternalResolution.y;

			switch (m_iInstanceID)
			{
			case 1:
				offset.x = emptySpaceX/2.0f/numHorizontalInstances;
				offset.y = emptySpaceY/2.0f/numVerticalInstances;
				break;
			case 2:
				offset.x = emptySpaceX/2.0f/numHorizontalInstances;
				offset.y = emptySpaceY/2.0f/numVerticalInstances + engine->getGraphics()->getResolution().y/2;
				break;
			case 3:
				offset.x = emptySpaceX/2.0f/numHorizontalInstances + engine->getGraphics()->getResolution().x/2;
				offset.y = emptySpaceY/2.0f/numVerticalInstances;
				break;
			case 4:
				offset.x = emptySpaceX/2.0f/numHorizontalInstances + engine->getGraphics()->getResolution().x/2;
				offset.y = emptySpaceY/2.0f/numVerticalInstances + engine->getGraphics()->getResolution().y/2;
				break;
			case 5:
				offset.x = emptySpaceX/2.0f/numHorizontalInstances + engine->getGraphics()->getResolution().x/2;
				offset.y = emptySpaceY/2.0f/numVerticalInstances + engine->getGraphics()->getResolution().y/2;
				break;
			case 6:
				offset.x = emptySpaceX/2.0f/numHorizontalInstances + engine->getGraphics()->getResolution().x/2;
				offset.y = emptySpaceY/2.0f/numVerticalInstances + engine->getGraphics()->getResolution().y/2;
				break;
			}
		}

		g->setBlending(false);
		{
			if (env->getOS() == Environment::OS::OS_HORIZON)
			{
				// NOTE: the nintendo switch always draws in 1080p, even undocked
				const Vector2 backupResolution = engine->getGraphics()->getResolution();
				g->onResolutionChange(Vector2(1920, 1080));
				{
					m_backBuffer->draw(g, offset.x*(1.0f + osu_letterboxing_offset_x.getFloat()), offset.y*(1.0f + osu_letterboxing_offset_y.getFloat()), g_vInternalResolution.x, g_vInternalResolution.y);
				}
				g->onResolutionChange(backupResolution);
			}
			else
			{
				if (osu_letterboxing.getBool())
					m_backBuffer->draw(g, offset.x*(1.0f + osu_letterboxing_offset_x.getFloat()), offset.y*(1.0f + osu_letterboxing_offset_y.getFloat()), g_vInternalResolution.x, g_vInternalResolution.y);
				else
				{
					if (osu_resolution_keep_aspect_ratio.getBool())
					{
						const float scale = getImageScaleToFitResolution(m_backBuffer->getSize(), engine->getGraphics()->getResolution());
						const float scaledWidth = m_backBuffer->getWidth()*scale;
						const float scaledHeight = m_backBuffer->getHeight()*scale;
						m_backBuffer->draw(g, std::max(0.0f, engine->getGraphics()->getResolution().x/2.0f - scaledWidth/2.0f)*(1.0f + osu_letterboxing_offset_x.getFloat()), std::max(0.0f, engine->getGraphics()->getResolution().y/2.0f - scaledHeight/2.0f)*(1.0f + osu_letterboxing_offset_y.getFloat()), scaledWidth, scaledHeight);
					}
					else
						m_backBuffer->draw(g, 0, 0, engine->getGraphics()->getResolution().x, engine->getGraphics()->getResolution().y);
				}
			}
		}
		g->setBlending(true);
	}

	// now, let OpenVR draw (this internally then calls the registered callback, meaning drawVR() here)
	if (isInVRMode())
		openvr->draw(g);
}

void Osu::drawVR(Graphics *g)
{
	m_bIsInVRDraw = true;

	Matrix4 mvp = openvr->getCurrentModelViewProjectionMatrix();

	// draw virtual screen + environment
	m_vr->drawVR(g, mvp, m_backBuffer);

	// draw everything in the correct order
	if (isInPlayMode()) // if we are playing a beatmap
	{
		m_vr->drawVRHUD(g, mvp, m_hud);
		m_vr->drawVRBeatmap(g, mvp, getSelectedBeatmap());
	}
	else // not playing
	{
		if (m_optionsMenu->shouldDrawVRDummyHUD())
			m_vr->drawVRHUDDummy(g, mvp, m_hud);

		m_vr->drawVRPlayfieldDummy(g, mvp);
	}

	m_bIsInVRDraw = false;
}

void Osu::update()
{
	const int wheelDelta = engine->getMouse()->getWheelDeltaVertical(); // HACKHACK: songbrowser focus

	if (m_skin != NULL)
		m_skin->update();

	m_windowManager->update();
	if (isInVRMode())
		m_vr->update();

	for (int i=0; i<m_screens.size(); i++)
	{
		m_screens[i]->update();
	}

	// main beatmap update
	m_bSeeking = false;
	if (isInPlayMode())
	{
		if (m_osu_mod_fposu_ref->getBool())
			m_fposu->update();

		getSelectedBeatmap()->update();

		// scrubbing/seeking
		if (m_bSeekKey)
		{
			if (!isInMultiplayer() || m_multiplayer->isServer())
			{
				m_bSeeking = true;
				const float percent = clamp<float>(engine->getMouse()->getPos().x/getScreenWidth(), 0.0f, 1.0f);
				if (engine->getMouse()->isLeftDown())
					getSelectedBeatmap()->seekPercentPlayable(percent);
				if (engine->getMouse()->isRightDown())
					m_fQuickSaveTime = clamp<float>((float)((getSelectedBeatmap()->getStartTimePlayable()+getSelectedBeatmap()->getLengthPlayable())*percent) / (float)getSelectedBeatmap()->getLength(), 0.0f, 1.0f);
			}
		}

		// skip button clicking
		if (getSelectedBeatmap()->isInSkippableSection() && !getSelectedBeatmap()->isPaused() && !m_bSeeking)
		{
			// TODO: make this on click only, and not if held too. this can also cause earrape while scrubbing
			bool isAnyOsuKeyDown = (m_bKeyboardKey1Down || m_bKeyboardKey2Down || m_bMouseKey1Down || m_bMouseKey2Down);
			bool isAnyVRKeyDown = isInVRMode() && !m_vr->isUIActive() && (openvr->getLeftController()->isButtonPressed(OpenVRController::BUTTON::BUTTON_STEAMVR_TOUCHPAD) || openvr->getRightController()->isButtonPressed(OpenVRController::BUTTON::BUTTON_STEAMVR_TOUCHPAD)
												|| openvr->getLeftController()->getTrigger() > 0.95f || openvr->getRightController()->getTrigger() > 0.95f);
			if (!m_hud->isVolumeOverlayBusy() && (engine->getMouse()->isLeftDown() || isAnyOsuKeyDown || isAnyVRKeyDown))
			{
				if (m_hud->getSkipClickRect().contains(engine->getMouse()->getPos()) || isAnyVRKeyDown)
					m_bSkipScheduled = true;
			}
		}

		// skipping
		if (m_bSkipScheduled)
		{
			if (getSelectedBeatmap()->isInSkippableSection() && !getSelectedBeatmap()->isPaused())
			{
				m_multiplayer->onServerPlayStateChange(OsuMultiplayer::STATE::SKIP);

				if (!isInMultiplayer() || m_multiplayer->isServer())
					getSelectedBeatmap()->skipEmptySection();
			}

			m_bSkipScheduled = false;
		}

		// quick retry timer
		if (m_bQuickRetryDown && m_fQuickRetryTime != 0.0f && engine->getTime() > m_fQuickRetryTime)
		{
			m_fQuickRetryTime = 0.0f;

			if (!isInMultiplayer() || m_multiplayer->isServer())
			{
				getSelectedBeatmap()->restart(true);
				getSelectedBeatmap()->update();
				m_pauseMenu->setVisible(false);
			}
		}
	}

	// asynchronous ui toggles
	// TODO: this is cancer, why did I even write this section
	if (m_bToggleModSelectionScheduled)
	{
		m_bToggleModSelectionScheduled = false;

		if (!isInPlayMode())
		{
			if (m_songBrowser2 != NULL)
				m_songBrowser2->setVisible(m_modSelector->isVisible());
		}

		m_modSelector->setVisible(!m_modSelector->isVisible());
	}
	if (m_bToggleSongBrowserScheduled)
	{
		m_bToggleSongBrowserScheduled = false;

		if (m_userStatsScreen->isVisible())
			m_userStatsScreen->setVisible(false);

		if (m_mainMenu->isVisible() && m_optionsMenu->isVisible())
			m_optionsMenu->setVisible(false);

		if (m_songBrowser2 != NULL)
			m_songBrowser2->setVisible(!m_songBrowser2->isVisible());

		m_mainMenu->setVisible(!(m_songBrowser2 != NULL && m_songBrowser2->isVisible()));
		updateConfineCursor();
	}
	if (m_bToggleOptionsMenuScheduled)
	{
		m_bToggleOptionsMenuScheduled = false;

		const bool fullscreen = (isInVRMode() ? m_bOptionsMenuFullscreen : false);
		const bool wasFullscreen = m_optionsMenu->isFullscreen();

		m_optionsMenu->setFullscreen(fullscreen);
		m_optionsMenu->setVisible(!m_optionsMenu->isVisible());
		if (fullscreen || wasFullscreen)
			m_mainMenu->setVisible(!m_optionsMenu->isVisible());
	}
	if (m_bToggleRankingScreenScheduled)
	{
		m_bToggleRankingScreenScheduled = false;

		m_rankingScreen->setVisible(!m_rankingScreen->isVisible());
		if (m_songBrowser2 != NULL && m_iInstanceID < 2)
			m_songBrowser2->setVisible(!m_rankingScreen->isVisible());
	}
	if (m_bToggleUserStatsScreenScheduled)
	{
		m_bToggleUserStatsScreenScheduled = false;

		if (m_iInstanceID < 2)
		{
			m_userStatsScreen->setVisible(true);

			if (m_songBrowser2 != NULL && m_songBrowser2->isVisible())
				m_songBrowser2->setVisible(false);
		}
	}
	if (m_bToggleVRTutorialScheduled)
	{
		m_bToggleVRTutorialScheduled = false;

		m_mainMenu->setVisible(!m_mainMenu->isVisible());
		m_vrTutorial->setVisible(!m_mainMenu->isVisible());
	}
	if (m_bToggleChangelogScheduled)
	{
		m_bToggleChangelogScheduled = false;

		m_mainMenu->setVisible(!m_mainMenu->isVisible());
		m_changelog->setVisible(!m_mainMenu->isVisible());
	}
	if (m_bToggleEditorScheduled)
	{
		m_bToggleEditorScheduled = false;

		m_mainMenu->setVisible(!m_mainMenu->isVisible());
		m_editor->setVisible(!m_mainMenu->isVisible());
	}

	// handle cursor visibility if outside of internal resolution
	// TODO: not a critical bug, but the real cursor gets visible way too early if sensitivity is > 1.0f, due to this using scaled/offset getMouse()->getPos()
	if (osu_resolution_enabled.getBool() && m_iInstanceID < 1)
	{
		McRect internalWindow = McRect(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
		bool cursorVisible = env->isCursorVisible();
		if (!internalWindow.contains(engine->getMouse()->getPos()))
		{
			if (!cursorVisible)
				env->setCursorVisible(true);
		}
		else
		{
			if (cursorVisible != m_bShouldCursorBeVisible)
				env->setCursorVisible(m_bShouldCursorBeVisible);
		}
	}

	// handle mousewheel volume change
	if ((m_songBrowser2 != NULL && (!m_songBrowser2->isVisible() || engine->getKeyboard()->isAltDown() || m_hud->isVolumeOverlayBusy()))
			&& (!m_optionsMenu->isVisible() || !m_optionsMenu->isMouseInside() || engine->getKeyboard()->isAltDown())
			&& !m_vrTutorial->isVisible()
			&& (!m_userStatsScreen->isVisible() || engine->getKeyboard()->isAltDown() || m_hud->isVolumeOverlayBusy())
			&& (!m_changelog->isVisible() || engine->getKeyboard()->isAltDown())
			&& (!m_modSelector->isMouseInScrollView() || engine->getKeyboard()->isAltDown()))
	{
		if ((!(isInPlayMode() && !m_pauseMenu->isVisible()) && !m_rankingScreen->isVisible()) || (isInPlayMode() && !osu_disable_mousewheel.getBool()) || engine->getKeyboard()->isAltDown())
		{
			if (wheelDelta != 0)
			{
				const int multiplier = std::max(1, std::abs(wheelDelta) / 120);

				if (wheelDelta > 0)
					volumeUp(multiplier);
				else
					volumeDown(multiplier);
			}
		}
	}

	// TODO: shitty override, but it works well enough for now => see OsuVR.cpp
	// it's a bit of a hack, because using cursor visibility to work around SetCursorPos() affecting the windows cursor in the Mouse class
	if (isInVRMode() && !env->isCursorVisible())
		env->setCursorVisible(true);

	// endless mod
	if (m_bScheduleEndlessModNextBeatmap)
	{
		m_bScheduleEndlessModNextBeatmap = false;
		m_songBrowser2->playNextRandomBeatmap();
	}

	// multiplayer update
	m_multiplayer->update();

	// skin async loading
	if (m_bSkinLoadScheduled)
	{
		if (m_skinScheduledToLoad != NULL && m_skinScheduledToLoad->isReady())
		{
			m_bSkinLoadScheduled = false;

			if (m_skin != m_skinScheduledToLoad)
				SAFE_DELETE(m_skin);

			m_skin = m_skinScheduledToLoad;

			m_skinScheduledToLoad = NULL;

			// force layout update after all skin elements have been loaded
			onResolutionChanged(g_vInternalResolution);

			// notify if done after reload
			if (m_bSkinLoadWasReload)
			{
				m_bSkinLoadWasReload = false;

				m_notificationOverlay->addNotification("Skin reloaded!");
			}
		}
	}
}

void Osu::updateMods()
{
	m_bModAuto = osu_mods.getString().find("auto") != -1;
	m_bModAutopilot = osu_mods.getString().find("autopilot") != -1;
	m_bModRelax = osu_mods.getString().find("relax") != -1;
	m_bModSpunout = osu_mods.getString().find("spunout") != -1;
	m_bModTarget = osu_mods.getString().find("practicetarget") != -1;
	m_bModScorev2 = osu_mods.getString().find("v2") != -1;
	m_bModDT = osu_mods.getString().find("dt") != -1;
	m_bModNC = osu_mods.getString().find("nc") != -1;
	m_bModNF = osu_mods.getString().find("nf") != -1;
	m_bModHT = osu_mods.getString().find("ht") != -1;
	m_bModDC = osu_mods.getString().find("dc") != -1;
	m_bModHD = osu_mods.getString().find("hd") != -1;
	m_bModHR = osu_mods.getString().find("hr") != -1;
	m_bModEZ = osu_mods.getString().find("ez") != -1;
	m_bModSD = osu_mods.getString().find("sd") != -1;
	m_bModSS = osu_mods.getString().find("ss") != -1;
	m_bModNM = osu_mods.getString().find("nm") != -1;

	// static overrides
	onSpeedChange("", osu_speed_override.getString());
	onPitchChange("", osu_pitch_override.getString());

	// autopilot overrides auto
	if (m_bModAutopilot)
		m_bModAuto = false;

	// handle auto/pilot cursor visibility
	if (!m_bModAuto && !m_bModAutopilot)
	{
		env->setCursorVisible(false);
		m_bShouldCursorBeVisible = false;
	}
	else if (isInPlayMode())
	{
		env->setCursorVisible(true);
		m_bShouldCursorBeVisible = true;
	}

	// notify the possibly running beatmap of mod changes, for e.g. recalculating stacks dynamically if HR is toggled
	if (getSelectedBeatmap() != NULL)
		getSelectedBeatmap()->onModUpdate();
}

void Osu::onKeyDown(KeyboardEvent &key)
{
	// global hotkeys

	// special hotkeys
	// reload & recompile shaders
	if (engine->getKeyboard()->isAltDown() && engine->getKeyboard()->isControlDown() && key == KEY_R)
	{
		Shader *sliderShader = engine->getResourceManager()->getShader("slider");
		Shader *sliderShaderVR = engine->getResourceManager()->getShader("sliderVR");
		Shader *cursorTrailShader = engine->getResourceManager()->getShader("cursortrail");

		if (sliderShader != NULL)
			sliderShader->reload();
		if (sliderShaderVR != NULL)
			sliderShaderVR->reload();
		if (cursorTrailShader != NULL)
			cursorTrailShader->reload();

		key.consume();
	}

	// reload skin (alt)
	if (engine->getKeyboard()->isAltDown() && engine->getKeyboard()->isControlDown() && key == KEY_S)
	{
		onSkinReload();
		key.consume();
	}

	// arrow keys volume (alt)
	if (engine->getKeyboard()->isAltDown() && key == (KEYCODE)OsuKeyBindings::INCREASE_VOLUME.getInt())
	{
		volumeUp();
		key.consume();
	}
	if (engine->getKeyboard()->isAltDown() && key == (KEYCODE)OsuKeyBindings::DECREASE_VOLUME.getInt())
	{
		volumeDown();
		key.consume();
	}

	// disable mouse buttons hotkey
	if (key == (KEYCODE)OsuKeyBindings::DISABLE_MOUSE_BUTTONS.getInt())
	{
		if (osu_disable_mousebuttons.getBool())
		{
			osu_disable_mousebuttons.setValue(0.0f);
			m_notificationOverlay->addNotification("Mouse buttons are enabled.");
		}
		else
		{
			osu_disable_mousebuttons.setValue(1.0f);
			m_notificationOverlay->addNotification("Mouse buttons are disabled.");
		}
	}

	// screenshots
	if (key == (KEYCODE)OsuKeyBindings::SAVE_SCREENSHOT.getInt())
		saveScreenshot();

	// boss key (minimize + mute)
	if (key == (KEYCODE)OsuKeyBindings::BOSS_KEY.getInt())
	{
		engine->getEnvironment()->minimize();
		if (getSelectedBeatmap() != NULL)
		{
			m_bWasBossKeyPaused = getSelectedBeatmap()->isPreviewMusicPlaying();
			getSelectedBeatmap()->pausePreviewMusic(false);
		}
	}

	// local hotkeys

	// while playing
	if (isInPlayMode() && !m_optionsMenu->isVisible())
	{
		// while playing and not paused
		if (!getSelectedBeatmap()->isPaused())
		{
			getSelectedBeatmap()->onKeyDown(key);

			if (!m_bKeyboardKey1Down && key == (KEYCODE)OsuKeyBindings::LEFT_CLICK.getInt())
			{
				m_bKeyboardKey1Down = true;
				onKey1Change(true, false);
				key.consume();
			}

			if (!m_bKeyboardKey2Down && key == (KEYCODE)OsuKeyBindings::RIGHT_CLICK.getInt())
			{
				m_bKeyboardKey2Down = true;
				onKey2Change(true, false);
				key.consume();
			}

			// handle skipping
			if (key == KEY_ENTER || key == (KEYCODE)OsuKeyBindings::SKIP_CUTSCENE.getInt())
				m_bSkipScheduled = true;

			// toggle ui
			if (key == KEY_SHIFT)
			{
				if (m_bTab && !m_bUIToggleCheck)
				{
					m_bUIToggleCheck = true;
					m_osu_draw_hud_ref->setValue(!m_osu_draw_hud_ref->getBool());

					key.consume();
				}
			}
			if (key == KEY_TAB)
			{
				m_bTab = true;
				if (engine->getKeyboard()->isShiftDown() && !m_bUIToggleCheck)
				{
					m_bUIToggleCheck = true;
					m_osu_draw_hud_ref->setValue(!m_osu_draw_hud_ref->getBool());

					key.consume();
				}
			}
			if (key == (KEYCODE)OsuKeyBindings::TOGGLE_SCOREBOARD.getInt() && !m_bScoreboardToggleCheck)
			{
				m_bScoreboardToggleCheck = true;
				m_osu_draw_scoreboard->setValue(!m_osu_draw_scoreboard->getBool());

				key.consume();
			}

			// allow live mod changing while playing
			if (!key.isConsumed() && key == KEY_F1 && !m_bF1 && !getSelectedBeatmap()->hasFailed()) // only if not failed though
			{
				m_bF1 = true;
				toggleModSelection(true);
			}

			// quick save/load
			if (!isInMultiplayer() || m_multiplayer->isServer())
			{
				if (key == (KEYCODE)OsuKeyBindings::QUICK_SAVE.getInt())
					m_fQuickSaveTime = getSelectedBeatmap()->getPercentFinished();
				if (key == (KEYCODE)OsuKeyBindings::QUICK_LOAD.getInt())
					getSelectedBeatmap()->seekPercent(m_fQuickSaveTime);
			}
		}

		// while paused or maybe not paused

		// handle quick restart
		if (((key == (KEYCODE)OsuKeyBindings::QUICK_RETRY.getInt() || (engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown() && key == KEY_R)) && !m_bQuickRetryDown))
		{
			m_bQuickRetryDown = true;
			m_fQuickRetryTime = engine->getTime() + osu_quick_retry_delay.getFloat();
		}

		// handle seeking
		if (key == (KEYCODE)OsuKeyBindings::SEEK_TIME.getInt())
			m_bSeekKey = true;
	}

	// forward to all subsystem, if not already consumed
	for (int i=0; i<m_screens.size(); i++)
	{
		if (key.isConsumed())
			break;

		m_screens[i]->onKeyDown(key);
	}

	// special handling, after subsystems, if still not consumed
	if (!key.isConsumed())
	{
		// if playing
		if (isInPlayMode())
		{
			// toggle pause menu
			if ((key == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt() || key == KEY_ESCAPE) && !m_bEscape)
			{
				if (!isInMultiplayer() || m_multiplayer->isServer() || m_iMultiplayerClientNumEscPresses > 1)
				{
					if (m_iMultiplayerClientNumEscPresses > 1)
					{
						getSelectedBeatmap()->stop(true);
					}
					else
					{
						if (!getSelectedBeatmap()->hasFailed() || !m_pauseMenu->isVisible()) // you can open the pause menu while the failing animation is happening, but never close it then
						{
							m_bEscape = true;

							getSelectedBeatmap()->pause();
							m_pauseMenu->setVisible(getSelectedBeatmap()->isPaused());

							key.consume();
						}
						else // pressing escape while in failed pause menu
						{
							getSelectedBeatmap()->stop(true);
						}
					}
				}
				else
				{
					m_iMultiplayerClientNumEscPresses++;
					if (m_iMultiplayerClientNumEscPresses == 2)
						m_notificationOverlay->addNotification("Hit 'Escape' once more to exit this multiplayer match.", 0xffffffff, false, 0.75f);
				}
			}

			// local offset
			if (key == (KEYCODE)OsuKeyBindings::INCREASE_LOCAL_OFFSET.getInt())
			{
				long offsetAdd = engine->getKeyboard()->isAltDown() ? 1 : 5;
				getSelectedBeatmap()->getSelectedDifficulty()->localoffset += offsetAdd;
				m_notificationOverlay->addNotification(UString::format("Local beatmap offset set to %ld ms", getSelectedBeatmap()->getSelectedDifficulty()->localoffset));
			}
			if (key == (KEYCODE)OsuKeyBindings::DECREASE_LOCAL_OFFSET.getInt())
			{
				long offsetAdd = -(engine->getKeyboard()->isAltDown() ? 1 : 5);
				getSelectedBeatmap()->getSelectedDifficulty()->localoffset += offsetAdd;
				m_notificationOverlay->addNotification(UString::format("Local beatmap offset set to %ld ms", getSelectedBeatmap()->getSelectedDifficulty()->localoffset));
			}

			// mania scroll speed
			/*
			if (key == (KEYCODE)OsuKeyBindings::INCREASE_SPEED.getInt())
			{
				ConVar *maniaSpeed = convar->getConVarByName("osu_mania_speed");
				maniaSpeed->setValue(clamp<float>(std::round((maniaSpeed->getFloat() + 0.05f) * 100.0f) / 100.0f, 0.05f, 10.0f));
				m_notificationOverlay->addNotification(UString::format("osu!mania speed set to %gx (fixed)", maniaSpeed->getFloat()));
			}
			if (key == (KEYCODE)OsuKeyBindings::DECREASE_SPEED.getInt())
			{
				ConVar *maniaSpeed = convar->getConVarByName("osu_mania_speed");
				maniaSpeed->setValue(clamp<float>(std::round((maniaSpeed->getFloat() - 0.05f) * 100.0f) / 100.0f, 0.05f, 10.0f));
				m_notificationOverlay->addNotification(UString::format("osu!mania speed set to %gx (fixed)", maniaSpeed->getFloat()));
			}
			*/
		}

		// if playing or not playing

		// volume
		if (key == (KEYCODE)OsuKeyBindings::INCREASE_VOLUME.getInt())
			volumeUp();
		if (key == (KEYCODE)OsuKeyBindings::DECREASE_VOLUME.getInt())
			volumeDown();

		// volume slider selection
		if (m_hud->isVolumeOverlayVisible())
		{
			if (key != (KEYCODE)OsuKeyBindings::INCREASE_VOLUME.getInt() && key != (KEYCODE)OsuKeyBindings::DECREASE_VOLUME.getInt())
			{
				if (key == KEY_LEFT)
					m_hud->selectVolumeNext();
				if (key == KEY_RIGHT)
					m_hud->selectVolumePrev();
			}
		}
	}
}

void Osu::onKeyUp(KeyboardEvent &key)
{
	if (isInPlayMode())
	{
		if (!getSelectedBeatmap()->isPaused())
		{
			getSelectedBeatmap()->onKeyUp(key);
		}
	}

	// clicks
	if (key == (KEYCODE)OsuKeyBindings::LEFT_CLICK.getInt() && m_bKeyboardKey1Down)
	{
		m_bKeyboardKey1Down = false;
		if (isInPlayMode())
			onKey1Change(false, false);
	}
	if (key == (KEYCODE)OsuKeyBindings::RIGHT_CLICK.getInt() && m_bKeyboardKey2Down)
	{
		m_bKeyboardKey2Down = false;
		if (isInPlayMode())
			onKey2Change(false, false);
	}

	// forward to all subsystems, if not consumed
	for (int i=0; i<m_screens.size(); i++)
	{
		if (key.isConsumed())
			break;

		m_screens[i]->onKeyUp(key);
	}

	// misc hotkeys release
	if (key == KEY_F1)
		m_bF1 = false;
	if (key == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt() || key == KEY_ESCAPE)
		m_bEscape = false;
	if (key == KEY_SHIFT)
		m_bUIToggleCheck = false;
	if (key == (KEYCODE)OsuKeyBindings::TOGGLE_SCOREBOARD.getInt())
		m_bScoreboardToggleCheck = false;
	if (key == KEY_TAB)
	{
		m_bTab = false;
		m_bUIToggleCheck = false;
	}
	if (key == (KEYCODE)OsuKeyBindings::QUICK_RETRY.getInt() || key == KEY_R)
		m_bQuickRetryDown = false;
	if (key == (KEYCODE)OsuKeyBindings::SEEK_TIME.getInt())
		m_bSeekKey = false;
}

void Osu::onChar(KeyboardEvent &e)
{
	for (int i=0; i<m_screens.size(); i++)
	{
		if (e.isConsumed())
			break;

		m_screens[i]->onChar(e);
	}
}

void Osu::onLeftChange(bool down)
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_disable_mousebuttons.getBool()) return;

	if (!m_bMouseKey1Down && down)
	{
		m_bMouseKey1Down = true;
		onKey1Change(true, true);
	}
	else if (m_bMouseKey1Down)
	{
		m_bMouseKey1Down = false;
		onKey1Change(false, true);
	}
}

void Osu::onRightChange(bool down)
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_disable_mousebuttons.getBool()) return;

	if (!m_bMouseKey2Down && down)
	{
		m_bMouseKey2Down = true;
		onKey2Change(true, true);
	}
	else if (m_bMouseKey2Down)
	{
		m_bMouseKey2Down = false;
		onKey2Change(false, true);
	}
}

void Osu::toggleModSelection(bool waitForF1KeyUp)
{
	m_bToggleModSelectionScheduled = true;
	m_modSelector->setWaitForF1KeyUp(waitForF1KeyUp);
}

void Osu::toggleSongBrowser()
{
	m_bToggleSongBrowserScheduled = true;
}

void Osu::toggleOptionsMenu()
{
	m_bToggleOptionsMenuScheduled = true;
	m_bOptionsMenuFullscreen = m_mainMenu->isVisible();
}

void Osu::toggleRankingScreen()
{
	m_bToggleRankingScreenScheduled = true;
}

void Osu::toggleUserStatsScreen()
{
	m_bToggleUserStatsScreenScheduled = true;
}

void Osu::toggleVRTutorial()
{
	m_bToggleVRTutorialScheduled = true;
}

void Osu::toggleChangelog()
{
	m_bToggleChangelogScheduled = true;
}

void Osu::toggleEditor()
{
	m_bToggleEditorScheduled = true;
}

void Osu::onVolumeChange(int multiplier)
{
	// chose which volume to change, depending on the volume overlay, default is master
	ConVar *volumeConVar = &osu_volume_master;
	if (m_hud->getVolumeMusicSlider()->isSelected())
		volumeConVar = &osu_volume_music;
	else if (m_hud->getVolumeEffectsSlider()->isSelected())
		volumeConVar = m_osu_volume_effects_ref;

	// change the volume
	if (m_hud->isVolumeOverlayVisible())
	{
		float newVolume = clamp<float>(volumeConVar->getFloat() + osu_volume_change_interval.getFloat()*multiplier, 0.0f, 1.0f);
		volumeConVar->setValue(newVolume);
	}

	m_hud->animateVolumeChange();
}

void Osu::onAudioOutputDeviceChange()
{
	if (getSelectedBeatmap() != NULL && getSelectedBeatmap()->getMusic() != NULL)
	{
		getSelectedBeatmap()->getMusic()->reload();
		getSelectedBeatmap()->select();
	}

	reloadSkin();
}

void Osu::saveScreenshot()
{
	engine->getSound()->play(m_skin->getShutter());
	int screenshotNumber = 0;
	while (env->fileExists(UString::format("screenshots/screenshot%i.png", screenshotNumber)))
	{
		screenshotNumber++;
	}
	std::vector<unsigned char> pixels = engine->getGraphics()->getScreenshot();
	Image::saveToImage(&pixels[0], engine->getGraphics()->getResolution().x, engine->getGraphics()->getResolution().y, UString::format("screenshots/screenshot%i.png", screenshotNumber));
}



void Osu::onBeforePlayStart()
{
	debugLog("Osu::onBeforePlayStart()\n");

	engine->getSound()->play(m_skin->getMenuHit());

	updateMods();

	// mp hack
	{
		m_mainMenu->setVisible(false);
		m_modSelector->setVisible(false);
		m_optionsMenu->setVisible(false);
		m_pauseMenu->setVisible(false);
		m_rankingScreen->setVisible(false);
	}
}

void Osu::onPlayStart()
{
	debugLog("Osu::onPlayStart()\n");

	m_snd_change_check_interval_ref->setValue(0.0f);

	if (m_bModAuto || m_bModAutopilot)
	{
		env->setCursorVisible(true);
		m_bShouldCursorBeVisible = true;
	}

	if (getSelectedBeatmap()->getSelectedDifficulty()->localoffset != 0)
		m_notificationOverlay->addNotification(UString::format("Using local beatmap offset (%ld ms)", getSelectedBeatmap()->getSelectedDifficulty()->localoffset), 0xffffffff, false, 0.75f);

	m_fQuickSaveTime = 0.0f; // reset

	updateConfineCursor();

	OsuRichPresence::onPlayStart(this);
}

void Osu::onPlayEnd(bool quit)
{
	debugLog("Osu::onPlayEnd()\n");

	OsuRichPresence::onPlayEnd(this, quit);

	m_snd_change_check_interval_ref->setValue(m_snd_change_check_interval_ref->getDefaultFloat());

	if (!quit)
	{
		if (!osu_mod_endless.getBool())
		{
			if (isInMultiplayer())
			{
				// score packets while playing are sent unreliably, but at the end we want to have reliably synced values across all players
				// also, at the end, we display the maximum achieved combo instead of the current one
				m_multiplayer->onClientScoreChange(m_score->getComboMax(), m_score->getAccuracy(), m_score->getScore(), m_score->isDead(), true);
			}

			m_rankingScreen->setScore(m_score);
			m_rankingScreen->setBeatmapInfo(getSelectedBeatmap(), getSelectedBeatmap()->getSelectedDifficulty());

			engine->getSound()->play(m_skin->getApplause());
		}
		else
		{
			m_bScheduleEndlessModNextBeatmap = true;
			return; // nothing more to do here
		}
	}

	m_mainMenu->setVisible(false);
	m_modSelector->setVisible(false);
	m_pauseMenu->setVisible(false);

	env->setCursorVisible(false);
	m_bShouldCursorBeVisible = false;

	m_iMultiplayerClientNumEscPresses = 0;

	if (m_songBrowser2 != NULL)
		m_songBrowser2->onPlayEnd(quit);

	if (quit)
	{
		if (m_iInstanceID < 2)
			toggleSongBrowser();
		else
			m_mainMenu->setVisible(true);
	}
	else
		toggleRankingScreen();

	updateConfineCursor();
}



OsuBeatmap *Osu::getSelectedBeatmap()
{
	if (m_songBrowser2 != NULL)
		return m_songBrowser2->getSelectedBeatmap();

	return NULL;
}

float Osu::getDifficultyMultiplier()
{
	float difficultyMultiplier = 1.0f;

	if (m_bModHR)
		difficultyMultiplier = 1.4f;
	if (m_bModEZ)
		difficultyMultiplier = 0.5f;

	return difficultyMultiplier;
}

float Osu::getCSDifficultyMultiplier()
{
	float difficultyMultiplier = 1.0f;

	if (m_bModHR)
		difficultyMultiplier = 1.3f; // different!
	if (m_bModEZ)
		difficultyMultiplier = 0.5f;

	return difficultyMultiplier;
}

float Osu::getScoreMultiplier()
{
	float multiplier = 1.0f;

	if (m_bModEZ || m_bModNF)
		multiplier *= 0.5f;
	if (m_bModHT || m_bModDC)
		multiplier *= 0.3f;
	if (m_bModHR)
		multiplier *= 1.06f;
	if (m_bModDT || m_bModNC)
		multiplier *= 1.12f;
	if (m_bModHD)
		multiplier *= 1.06f;
	if (m_bModSpunout)
		multiplier *= 0.9f;

	return multiplier;
}

float Osu::getRawSpeedMultiplier()
{
	float speedMultiplier = 1.0f;

	if (m_bModDT || m_bModNC || m_bModHT || m_bModDC)
	{
		if (m_bModDT || m_bModNC)
			speedMultiplier = 1.5f;
		else
			speedMultiplier = 0.75f;
	}

	return speedMultiplier;
}

float Osu::getSpeedMultiplier()
{
	float speedMultiplier = getRawSpeedMultiplier();

	if (osu_speed_override.getFloat() >= 0.0f)
		return std::max(osu_speed_override.getFloat(), 0.05f);

	return speedMultiplier;
}

float Osu::getPitchMultiplier()
{
	float pitchMultiplier = 1.0f;

	if (m_bModDC)
		pitchMultiplier = 0.92f;

	if (m_bModNC)
		pitchMultiplier = 1.1166f;

	if (osu_pitch_override.getFloat() > 0.0f)
		return osu_pitch_override.getFloat();

	return pitchMultiplier;
}

bool Osu::isInPlayMode()
{
	return (m_songBrowser2 != NULL && m_songBrowser2->hasSelectedAndIsPlaying());
}

bool Osu::isNotInPlayModeOrPaused()
{
	return !isInPlayMode() || (getSelectedBeatmap() != NULL && getSelectedBeatmap()->isPaused());
}

bool Osu::isInVRMode()
{
	return (osu_vr.getBool() && openvr->isReady());
}

bool Osu::isInMultiplayer()
{
	return m_multiplayer->isInMultiplayer();
}

bool Osu::shouldFallBackToLegacySliderRenderer()
{
	return osu_force_legacy_slider_renderer.getBool()
			|| m_osu_mod_wobble_ref->getBool()
			|| m_osu_mod_wobble2_ref->getBool()
			|| m_osu_mod_minimize_ref->getBool()
			|| m_modSelector->isCSOverrideSliderActive()
			/* || (m_osu_playfield_rotation->getFloat() < -0.01f || m_osu_playfield_rotation->getFloat() > 0.01f)*/;
}



void Osu::onResolutionChanged(Vector2 newResolution)
{
	if (engine->isMinimized()) return; // ignore if minimized

	if (m_iInstanceID < 1)
	{
		if (!osu_resolution_enabled.getBool())
			g_vInternalResolution = newResolution;
		else if (!engine->isMinimized()) // if we just got minimized, ignore the resolution change (for the internal stuff)
		{
			// clamp upwards to internal resolution (osu_resolution)
			if (g_vInternalResolution.x < m_vInternalResolution.x)
				g_vInternalResolution.x = m_vInternalResolution.x;
			if (g_vInternalResolution.y < m_vInternalResolution.y)
				g_vInternalResolution.y = m_vInternalResolution.y;

			// clamp downwards to engine resolution
			if (newResolution.x < g_vInternalResolution.x)
				g_vInternalResolution.x = newResolution.x;
			if (newResolution.y < g_vInternalResolution.y)
				g_vInternalResolution.y = newResolution.y;

			// disable internal resolution on specific conditions
			bool windowsBorderlessHackCondition = (env->getOS() == Environment::OS::OS_WINDOWS && env->isFullscreen() && env->isFullscreenWindowedBorderless() && (int)g_vInternalResolution.y == (int)env->getNativeScreenSize().y); // HACKHACK
			if (((int)g_vInternalResolution.x == engine->getScreenWidth() && (int)g_vInternalResolution.y == engine->getScreenHeight()) || !env->isFullscreen() || windowsBorderlessHackCondition)
			{
				debugLog("Internal resolution == Engine resolution || !Fullscreen, disabling resampler (%i, %i)\n", (int)(g_vInternalResolution == engine->getScreenSize()), (int)(!env->isFullscreen()));
				osu_resolution_enabled.setValue(0.0f);
				g_vInternalResolution = engine->getScreenSize();
			}
		}
	}

	// interfaces
	for (int i=0; i<m_screens.size(); i++)
	{
		m_screens[i]->onResolutionChange(g_vInternalResolution);
	}

	// rendertargets
	rebuildRenderTargets();

	// mouse scale/offset
	updateMouseSettings();

	// cursor clipping
	updateConfineCursor();
}

void Osu::rebuildRenderTargets()
{
	debugLog("Osu(%i)::rebuildRenderTargets: %fx%f\n", m_iInstanceID, g_vInternalResolution.x, g_vInternalResolution.y);

	m_backBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
	m_sliderFrameBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);

	if (m_osu_mod_fposu_ref->getBool())
		m_playfieldBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
	else
		m_playfieldBuffer->rebuild(0, 0, 64, 64);

	if (m_osu_mod_mafham_ref->getBool())
	{
		m_frameBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
		m_frameBuffer2->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
	}
	else
	{
		m_frameBuffer->rebuild(0, 0, 64, 64);
		m_frameBuffer2->rebuild(0, 0, 64, 64);
	}
}

void Osu::updateMouseSettings()
{
	// mouse scaling & offset
	Vector2 offset = Vector2(0, 0);
	Vector2 scale = Vector2(1, 1);
	if (osu_resolution_enabled.getBool())
	{
		if (osu_letterboxing.getBool())
		{
			offset = -Vector2((engine->getScreenWidth()/2 - g_vInternalResolution.x/2)*(1.0f + osu_letterboxing_offset_x.getFloat()), (engine->getScreenHeight()/2 - g_vInternalResolution.y/2)*(1.0f + osu_letterboxing_offset_y.getFloat()));
			scale = Vector2(g_vInternalResolution.x / engine->getScreenWidth(), g_vInternalResolution.y / engine->getScreenHeight());
		}
	}

	engine->getMouse()->setOffset(offset);
	engine->getMouse()->setScale(scale);
}

void Osu::onInternalResolutionChanged(UString oldValue, UString args)
{
	if (args.length() < 7) return;

	std::vector<UString> resolution = args.split("x");
	if (resolution.size() != 2)
		debugLog("Error: Invalid parameter count for command 'osu_resolution'! (Usage: e.g. \"osu_resolution 1280x720\")");
	else
	{
		int width = resolution[0].toFloat();
		int height = resolution[1].toFloat();

		if (width < 300 || height < 240)
			debugLog("Error: Invalid values for resolution for command 'osu_resolution'!");
		else
		{
			Vector2 newInternalResolution = Vector2(width, height);

			// clamp requested internal resolution to current renderer resolution
			// however, this could happen while we are transitioning into fullscreen. therefore only clamp when not in fullscreen or not in fullscreen transition
			bool isTransitioningIntoFullscreenHack = engine->getGraphics()->getResolution().x < env->getNativeScreenSize().x || engine->getGraphics()->getResolution().y < env->getNativeScreenSize().y;
			if (!env->isFullscreen() || !isTransitioningIntoFullscreenHack)
			{
				if (newInternalResolution.x > engine->getGraphics()->getResolution().x)
					newInternalResolution.x = engine->getGraphics()->getResolution().x;
				if (newInternalResolution.y > engine->getGraphics()->getResolution().y)
					newInternalResolution.y = engine->getGraphics()->getResolution().y;
			}

			// enable and store, then force onResolutionChanged()
			osu_resolution_enabled.setValue(1.0f);
			g_vInternalResolution = newInternalResolution;
			m_vInternalResolution = newInternalResolution;
			onResolutionChanged(newInternalResolution);
		}
	}
}

void Osu::onFocusGained()
{
	// cursor clipping
	updateConfineCursor();

	if (m_bWasBossKeyPaused)
	{
		m_bWasBossKeyPaused = false;
		if (getSelectedBeatmap() != NULL)
			getSelectedBeatmap()->pausePreviewMusic();
	}
}

void Osu::onFocusLost()
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_pause_on_focus_loss.getBool())
	{
		if (!isInMultiplayer())
		{
			getSelectedBeatmap()->pause(false);
			m_pauseMenu->setVisible(true);
			m_modSelector->setVisible(false);
		}
	}

	// release cursor clip
	env->setCursorClip(false, McRect());
}

bool Osu::onShutdown()
{
	debugLog("Osu::onShutdown()\n");

	// save everything
	m_optionsMenu->save();
	m_songBrowser2->getDatabase()->save();

	// the only time where a shutdown could be problematic is while an update is being installed, so we block it here
	return m_updateHandler == NULL || m_updateHandler->getStatus() != OsuUpdateHandler::STATUS::STATUS_INSTALLING_UPDATE;
}

void Osu::onSkinReload()
{
	m_bSkinLoadWasReload = true;
	onSkinChange("", osu_skin.getString());
}

void Osu::onSkinChange(UString oldValue, UString newValue)
{
	if (m_skin != NULL)
	{
		if (m_bSkinLoadScheduled || m_skinScheduledToLoad != NULL) return;
		if (newValue.length() < 1) return;
	}

	UString skinFolder = m_osu_folder_ref->getString();
	skinFolder.append(m_osu_folder_sub_skins_ref->getString());
	skinFolder.append(newValue);
	skinFolder.append("/");

	// reset playtime tracking
	steam->stopWorkshopPlaytimeTrackingForAllItems();

	// workshop skins use absolute paths
	const bool isWorkshopSkin = osu_skin_is_from_workshop.getBool();
	if (isWorkshopSkin)
	{
		skinFolder = newValue;

		// ensure that the skinFolder ends with a slash
		if (skinFolder[skinFolder.length()-1] != L'/' && skinFolder[skinFolder.length()-1] != L'\\')
			skinFolder.append("/");

		// start playtime tracking
		steam->startWorkshopItemPlaytimeTracking((uint64_t)osu_skin_workshop_id.getString().toLong());
	}

	m_skinScheduledToLoad = new OsuSkin(this, skinFolder, (newValue == "default" || newValue == "defaultvr"), isWorkshopSkin);

	// initial load
	if (m_skin == NULL)
		m_skin = m_skinScheduledToLoad;

	m_bSkinLoadScheduled = true;
}

void Osu::onMasterVolumeChange(UString oldValue, UString newValue)
{
	float newVolume = newValue.toFloat();
	engine->getSound()->setVolume(newVolume);
}

void Osu::onMusicVolumeChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
		getSelectedBeatmap()->setVolume(newValue.toFloat());
}

void Osu::onSpeedChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
	{
		float speed = newValue.toFloat();
		getSelectedBeatmap()->setSpeed(speed >= 0.0f ? speed : getSpeedMultiplier());
	}
}

void Osu::onPitchChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
	{
		float pitch = newValue.toFloat();
		getSelectedBeatmap()->setPitch(pitch > 0.0f ? pitch : getPitchMultiplier());
	}
}

void Osu::onPlayfieldChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
		getSelectedBeatmap()->onModUpdate();
}

void Osu::onLetterboxingChange(UString oldValue, UString newValue)
{
	if (osu_resolution_enabled.getBool())
	{
		bool oldVal = oldValue.toFloat() > 0.0f;
		bool newVal = newValue.toFloat() > 0.0f;

		if (oldVal != newVal)
			Osu::onResolutionChanged(g_vInternalResolution);
	}
}

void Osu::updateConfineCursor()
{
	if (isInVRMode() || m_iInstanceID > 0) return;

	if ((osu_confine_cursor_fullscreen.getBool() && env->isFullscreen()) || (osu_confine_cursor_windowed.getBool() && !env->isFullscreen()) || (isInPlayMode() && !getSelectedBeatmap()->isPaused() && !getModAuto() && !getModAutopilot()))
		env->setCursorClip(true, McRect());
	else
		env->setCursorClip(false, McRect());
}

void Osu::onConfineCursorWindowedChange(UString oldValue, UString newValue)
{
	updateConfineCursor();
}

void Osu::onConfineCursorFullscreenChange(UString oldValue, UString newValue)
{
	updateConfineCursor();
}

void Osu::onKey1Change(bool pressed, bool mouse)
{
	// WARNING: if paused, keyReleased*() will be called out of sequence every time due to the fix. do not put actions in it
	if (isInPlayMode()/* && !getSelectedBeatmap()->isPaused()*/) // NOTE: allow keyup even while beatmap is paused, to correctly not-continue immediately due to pressed keys
	{
		if (!(mouse && osu_disable_mousebuttons.getBool()))
		{
			// quickfix
			if (osu_disable_mousebuttons.getBool())
				m_bMouseKey1Down = false;

			if (pressed && !(m_bKeyboardKey1Down && m_bMouseKey1Down) && !getSelectedBeatmap()->isPaused()) // see above note
				getSelectedBeatmap()->keyPressed1(mouse);
			else if (!m_bKeyboardKey1Down && !m_bMouseKey1Down)
				getSelectedBeatmap()->keyReleased1(mouse);
		}
	}

	// cursor anim + ripples
	const bool doAnimate = !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouse && osu_disable_mousebuttons.getBool());
	if (doAnimate)
	{
		if (pressed && !(m_bKeyboardKey1Down && m_bMouseKey1Down))
		{
			m_hud->animateCursorExpand();
			m_hud->addCursorRipple(engine->getMouse()->getPos());
		}
		else if (!m_bKeyboardKey1Down && !m_bMouseKey1Down && !m_bKeyboardKey2Down && !m_bMouseKey2Down)
			m_hud->animateCursorShrink();
	}
}

void Osu::onKey2Change(bool pressed, bool mouse)
{
	// WARNING: if paused, keyReleased*() will be called out of sequence every time due to the fix. do not put actions in it
	if (isInPlayMode()/* && !getSelectedBeatmap()->isPaused()*/) // NOTE: allow keyup even while beatmap is paused, to correctly not-continue immediately due to pressed keys
	{
		if (!(mouse && osu_disable_mousebuttons.getBool()))
		{
			// quickfix
			if (osu_disable_mousebuttons.getBool())
				m_bMouseKey2Down = false;

			if (pressed && !(m_bKeyboardKey2Down && m_bMouseKey2Down) && !getSelectedBeatmap()->isPaused()) // see above note
				getSelectedBeatmap()->keyPressed2(mouse);
			else if (!m_bKeyboardKey2Down && !m_bMouseKey2Down)
				getSelectedBeatmap()->keyReleased2(mouse);
		}
	}

	// cursor anim + ripples
	const bool doAnimate = !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouse && osu_disable_mousebuttons.getBool());
	if (doAnimate)
	{
		if (pressed && !(m_bKeyboardKey2Down && m_bMouseKey2Down))
		{
			m_hud->animateCursorExpand();
			m_hud->addCursorRipple(engine->getMouse()->getPos());
		}
		else if (!m_bKeyboardKey2Down && !m_bMouseKey2Down && !m_bKeyboardKey1Down && !m_bMouseKey1Down)
			m_hud->animateCursorShrink();
	}
}

void Osu::onModMafhamChange(UString oldValue, UString newValue)
{
	rebuildRenderTargets();
}

void Osu::onModFPoSuChange(UString oldValue, UString newValue)
{
	rebuildRenderTargets();
}

void Osu::onLetterboxingOffsetChange(UString oldValue, UString newValue)
{
	updateMouseSettings();
}

void Osu::onNotification(UString args)
{
	m_notificationOverlay->addNotification(args);
}



float Osu::getImageScaleToFitResolution(Vector2 size, Vector2 resolution)
{
	return resolution.x/size.x > resolution.y/size.y ? resolution.y/size.y : resolution.x/size.x;
}

float Osu::getImageScaleToFitResolution(Image *img, Vector2 resolution)
{
	return getImageScaleToFitResolution(Vector2(img->getWidth(), img->getHeight()), resolution);
}

float Osu::getImageScaleToFillResolution(Vector2 size, Vector2 resolution)
{
	return resolution.x/size.x < resolution.y/size.y ? resolution.y/size.y : resolution.x/size.x;
}

float Osu::getImageScaleToFillResolution(Image *img, Vector2 resolution)
{
	return getImageScaleToFillResolution(Vector2(img->getWidth(), img->getHeight()), resolution);
}

float Osu::getImageScale(Osu *osu, Vector2 size, float osuSize)
{
	int swidth = osu->getScreenWidth();
	int sheight = osu->getScreenHeight();

	if (swidth * 3 > sheight * 4)
		swidth = sheight * 4 / 3;
	else
		sheight = swidth * 3 / 4;

	const float xMultiplier = swidth / osuBaseResolution.x;
	const float yMultiplier = sheight / osuBaseResolution.y;

	const float xDiameter = osuSize*xMultiplier;
	const float yDiameter = osuSize*yMultiplier;

	return xDiameter/size.x > yDiameter/size.y ? xDiameter/size.x : yDiameter/size.y;
}

float Osu::getImageScale(Osu *osu, Image *img, float osuSize)
{
	return getImageScale(osu, Vector2(img->getWidth(), img->getHeight()), osuSize);
}

float Osu::getUIScale(Osu *osu, float osuResolutionRatio)
{
	int swidth = osu->getScreenWidth();
	int sheight = osu->getScreenHeight();

	if (swidth * 3 > sheight * 4)
		swidth = sheight * 4 / 3;
	else
		sheight = swidth * 3 / 4;

	const float xMultiplier = swidth / osuBaseResolution.x;
	const float yMultiplier = sheight / osuBaseResolution.y;

	const float xDiameter = osuResolutionRatio*xMultiplier;
	const float yDiameter = osuResolutionRatio*yMultiplier;

	return xDiameter > yDiameter ? xDiameter : yDiameter;
}

bool Osu::findIgnoreCase(const std::string &haystack, const std::string &needle)
{
	auto it = std::search(
	    haystack.begin(), haystack.end(),
		needle.begin(),   needle.end(),
	    [](char ch1, char ch2)
		{return std::tolower(ch1) == std::tolower(ch2);}
	);

	return (it != haystack.end());
}
