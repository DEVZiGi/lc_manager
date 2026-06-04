# LC Manager (v0.1 Alpha)

An external Quality of Life (QoL) dashboard and overlay tool for **Lobotomy Corporation**, built with **C++**, **ImGui**, and **GLFW**[cite: 2].

The tool establishes a live data pipeline via memory injection (`lc_manager_sdk`) to extract facility state into a unified UI without putting unnecessary performance strain on the game engine.

## Current Features (v0.1)
* **Status Dashboard:** Live monitoring of game state, injection status, and database connectivity.
* **Agent Tracker:** Detailed list of personnel, dynamic division assignments, and live stat calculations (factoring in E.G.O Gift bonuses).
* **Inventory Overview:** A structured, searchable catalog of all extracted Weapons, Armors, and E.G.O Gifts within the facility.
* **Ordeal Schedule:** Live breakdown of current and upcoming facility threats.
* **Agent Level Calculator:** Built-in tool to calculate missing EXP and points required for employee stat tier upgrades and promotions.

## Project Status & Future
This is an **early Alpha version (v0.1)**[cite: 2]. The core pipeline and basic UI components are fully operational. The application is planned for further expansion and active development.

Future updates may include:
* Advanced success rate prediction panels using exact game formulas.
* Safe abnormality extraction helper tools.
* Extended filters to eliminate layout clutter and streamline data tracking.

## Technical Stack
* **Language:** C++17[cite: 2]
* **UI Framework:** Dear ImGui[cite: 2]
* **Window/Context:** GLFW3[cite: 2]
* **JSON Parser:** nlohmann/json[cite: 2]
* **Injection Interface:** SharpMonoInjector / C# SDK backend[cite: 2]

## License
WTFPL – Do What The Fuck You Want To Public License (or do whatever you want with it).
