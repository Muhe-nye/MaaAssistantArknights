#include "RoguelikeCiphertextBoardGainTaskPlugin.h"

#include "Controller/Controller.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

bool asst::RoguelikeCiphertextBoardGainTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (m_roguelike_theme.empty()) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    if (m_roguelike_theme != "Sami") {
        return false;
    }
    const std::string roguelike_name = m_roguelike_theme + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;

    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    m_ocr_next_level = false;
    if (task_view == "Roguelike@CiphertextBoardGain") {
        m_ocr_after_combat = false;
        return true;
    }
    if (task_view == "Roguelike@StageEncounterSpecialClose") {
        m_ocr_after_combat = false;
        return true;
    }
    if (task_view == "Roguelike@GetDropSelectReward2") {
        m_ocr_after_combat = true;
        return true;
    }
    if (task_view == "Roguelike@NextLevel") {
        m_ocr_next_level = true;
        return true;
    }
    /* 可能是没点科技树会单独掉一个密文板，很快能点出来，暂时不做识别
    if (task_view == "Roguelike@GetDrop2") {
        m_ocr_after_combat = true;
        return true;
    }*/
    else {
        return false;
    }
}

bool asst::RoguelikeCiphertextBoardGainTaskPlugin::_run()
{
    LogTraceFunction;

    std::string theme = m_roguelike_theme;
    std::string mode = status()->get_properties(Status::RoguelikeMode).value();

    auto image = ctrler()->get_image();

    OCRer analyzer(image);
    std::string ciphertext_board = "None";
    if (m_ocr_next_level) {
        analyzer.set_task_info(theme + "@Roguelike@CiphertextBoardGainOcrNextLevel");
        if (analyzer.analyze()) {
            ciphertext_board = analyzer.get_result().front().text;
        }
        store_to_status(ciphertext_board, Status::RoguelikeCiphertextBoardFloor);
        json::array ciphertext_board_floor_array = get_array(Status::RoguelikeCiphertextBoardFloor);
        // 到达第二层后，获得上一层预见的密文板
        if (ciphertext_board_floor_array.size() >= 2) {
            std::string ciphertext_board_last_floor = (ciphertext_board_floor_array.end() - 2)->to_string();
            if (ciphertext_board_last_floor.size() >= 2 && ciphertext_board_last_floor.front() == '"' &&
                ciphertext_board_last_floor.back() == '"') {
                ciphertext_board_last_floor =
                    ciphertext_board_last_floor.substr(1, ciphertext_board_last_floor.size() - 2);
            }
            if (ciphertext_board_last_floor != "None") {
                store_to_status(ciphertext_board_last_floor, Status::RoguelikeCiphertextBoardOverview);
            }
        }
        return true;
    };

    std::string task_name = m_ocr_after_combat ? theme + "@Roguelike@CiphertextBoardGainOcrAfterCombat"
                                               : theme + "@Roguelike@CiphertextBoardGainOcr";
    analyzer.set_task_info(task_name);

    if (!analyzer.analyze()) {
        return false;
    }
    ciphertext_board = analyzer.get_result().front().text;
    store_to_status(ciphertext_board, Status::RoguelikeCiphertextBoardOverview);
    if (m_ocr_after_combat) {
        ctrler()->click(analyzer.get_result().front().rect);
    }

    return true;
}

bool asst::RoguelikeCiphertextBoardGainTaskPlugin::store_to_status(std::string ciphertext_board, auto& status_string)
{
    json::array overview = get_array(status_string);
    overview.emplace_back(ciphertext_board);
    status()->set_str(status_string, overview.to_string());
    return true;
}

json::array asst::RoguelikeCiphertextBoardGainTaskPlugin::get_array(auto& status_string)
{
    std::string overview_str = status()->get_str(status_string).value_or(json::value().to_string());
    json::value overview_json = json::parse(overview_str).value_or(json::value());
    return overview_json.as_array();
}
