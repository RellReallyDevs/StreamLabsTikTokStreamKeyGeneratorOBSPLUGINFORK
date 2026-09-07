#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>

#include "TikTokDock.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("tiktok-live-dock", "en-US")

#define PLUGIN_VERSION "1.0.0"

static TikTokDock *g_dock = nullptr;

MODULE_EXPORT const char *obs_module_description(void)
{
	return "All-in-one TikTok LIVE dock: auto stream key, title, and game "
	       "category via the Streamlabs TikTok LIVE API.";
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return "TikTok Live Dock";
}

bool obs_module_load(void)
{
	QMainWindow *main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());

	if (!main_window) {
		blog(LOG_WARNING, "[tiktok-live-dock] no OBS main window yet, aborting load");
		return false;
	}

	g_dock = new TikTokDock(main_window);
	obs_frontend_add_dock_by_id("TikTokLiveDock", "TikTok Live", g_dock);

	blog(LOG_INFO, "[tiktok-live-dock] plugin loaded (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[tiktok-live-dock] plugin unloaded");
}
