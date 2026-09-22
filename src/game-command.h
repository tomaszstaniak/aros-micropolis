#ifndef MICROPOLIS_GAME_COMMAND_H
#define MICROPOLIS_GAME_COMMAND_H
enum class GameCommand {
    None=0,About,Save,SaveAs,Load,Quit,
    AutoBudget,AutoBulldoze,DisastersEnabled,Sound,Animation,Messages,Notices,
    Monster,Fire,Flood,Meltdown,Tornado,Earthquake,
    Pause,Slow,Medium,Fast,
    Budget,Evaluation,Graphs,Overview,MessageHistory,DisplayToggle,DisplayMode,
    ChooseCity,ZoomIn,ZoomOut,ZoomNormal,ChalkOverlay
};
inline GameCommand gameCommandForRawKey(unsigned code) {
    switch(code){
        case 0x21:return GameCommand::SaveAs;
        case 0x28:return GameCommand::Load;
        case 0x45:return GameCommand::Quit;
        case 0x51:return GameCommand::Budget;
        case 0x52:return GameCommand::Evaluation;
        case 0x53:return GameCommand::MessageHistory;
        case 0x54:return GameCommand::Graphs;
        case 0x56:return GameCommand::Overview;
        case 0x59:return GameCommand::DisplayToggle;
        case 0x58:return GameCommand::DisplayMode;
        // Main-keyboard -/= (+ without Shift), keypad -/+, and 0 for 100%.
        case 0x0c:case 0x5e:return GameCommand::ZoomIn;
        case 0x0b:case 0x4a:return GameCommand::ZoomOut;
        case 0x0a:case 0x0f:return GameCommand::ZoomNormal;
        default:return GameCommand::None;
    }
}
constexpr int gameCommandCode(GameCommand command){return 0x200+(int)command;}
inline GameCommand decodeGameCommand(int code){return code>=0x200?(GameCommand)(code-0x200):gameCommandForRawKey((unsigned)code);}
#endif
