#pragma once

#include <wx/arrstr.h>
#include <wx/dnd.h>
#include <wx/dragimag.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/msw/textctrl.h>
#include <wx/overlay.h>
#include <wx/sizer.h>
#include <wx/wx.h>

#include <string>
#include <vector>

#include "ParallaxGenConfig.hpp"

#include "NIFUtil.hpp"

class ParallaxGenUI {
public:
  static bool UIExitTriggered; /** If this is true than the UI triggered an exit, which means the CLI should NOT wait
                                  for user to press ENTER */

  /**
   * @brief Initialize the wxWidgets UI framwork
   */
  static void init();

  /**
   * @brief Shows the launcher dialog to the user (Hangs thread until user presses okay)
   *
   * @param OldParams Params to show in the UI
   * @return ParallaxGenConfig::PGParams Params set by the user
   */
  static auto showLauncher(const ParallaxGenConfig::PGParams &OldParams) -> ParallaxGenConfig::PGParams;

  /**
   * @brief Shows the mod selection dialog to the user (Hangs thread until user presses okay)
   *
   * @param Conflicts Mod conflict map
   * @param ExistingMods Mods that already exist in the order
   * @return std::vector<std::wstring> Vector of mods sorted by the user
   */
  static auto selectModOrder(
      const std::unordered_map<std::wstring,
                               std::tuple<std::set<NIFUtil::ShapeShader>, std::unordered_set<std::wstring>>> &Conflicts,
      const std::vector<std::wstring> &ExistingMods) -> std::vector<std::wstring>;
};
