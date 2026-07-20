# QML 重构计划 — touhoukill (qt6_ui 分支)

> 本文件是 QML 重构的主索引：项目结构、任务清单、设计约定、踩坑记录、进度。
> **维护指示：每次大幅度更新（新增功能模块/改动桥接层/完成一个 notify 处理或 UI 组件/重命名）后，自主更新本文件的"任务清单"状态、"进度记录"与"目录结构"，不等用户提醒。** 小幅格式调整不必记。

## 0. 全局禁令

- **不使用 Qt5Compat**：全局禁用 `QT += 5compat`（`QSanguosha.pro` 当前 `QT += network widgets quick quickwidgets`，不得添加 `5compat`，已在 .pro 注释标注）。所有需要的效果/组件用 Qt 6 原生方案（如 `MultiEffect` 走 `import QtQuick.Effects`），**不得引入 `Qt5Compat.GraphicalEffects` 等 compat 模块**。
- **QML 与 CPP 文件纯 ASCII**：所有 `.qml`/`.cpp`/`.h`/`.pro` 文件（含注释、字符串）必须使用纯 ASCII 字符，**不得包含中文、全角符号、em dash（`—`）、`§` 等非 ASCII 字符**。中文说明写在 `plan.md`（本文件）里，代码注释用英文。em dash 用 `--` 替代。

## 1. 项目概述
将旧版基于 QGraphics 的界面 `src/uibackup/`（35 对 .cpp/.h，约 17k 行，不编译，仅参考）重构为 Qt 6 QML。`MainWindow` 通过 `QQuickWidget` 加载 `qml/main.qml`，在 StartScene 与 RoomScene 间切换。核心布局（Photo/Dashboard/CardItem/StartScene/RoomScene）已成型，游戏交互（弹窗/聊天/技能按钮/卡牌选择）逐步移植中。

技术栈：Qt 6 QML（QtQuick 6.5），`import rocks.touhousatsu 1.0`；`QQuickWidget` + `qmlRegisterType`/`qmlRegisterSingletonType`/`qmlRegisterUncreatableType`；qmake（`QSanguosha.pro`）。

## 2. 项目结构

### C++ 侧
- `src/dialog/mainwindow.cpp`：构造 `QQuickWidget`，`setContextProperty` 暴露 `MainWindowInstance`/`Sanguosha`/`Config`，`setSource("qml/main.qml")`。提供 `qml_switchToRoomScene()` 信号。全局 `QPointer<MainWindow> MainWindowInstance`（仿 `RoomSceneInstance`，构造函数第一行赋值）。
- `src/qmlui/qmlui.cpp`：`Q_COREAPP_STARTUP_FUNCTION(registerCore)` 自动注册 QML 类型。
  - 单例：`G`（`TouhouKillQmlUiGlobal`，字体/游戏模式判断、`getAssetUrl(path)`/`assetExists(path)` 资源访问）、`ServerInfo`（含 `EnableAI`/`GameMode`/`FreeChoose` 等）。
  - 可创建：`CppRoomScene`（`RoomScene`，`QQuickItem` 桥接宿主）。
  - uncreatable：`Card`/`Player`/`ClientPlayer`/`General`/`Client`/`Skill`/`ViewAsSkill`/`FilterSkill`/`ProhibitSkill`/`DistanceSkill`/`MaxCardsSkill`/`TargetModSkill`/`AttackRangeSkill`。
- `src/qmlui/roomscene.h/cpp`：桥接层。`Q_PROPERTY` 暴露 `Self`（`ClientPlayer *`）、`ClientInstance`（`Client *`）；`connectClientSignals()` 集中连接 Client 信号；`Q_INVOKABLE replyToServer/notifyServer/freeChooseGeneral`。全局 `QPointer<RoomScene> RoomSceneInstance`。
- `src/client/client.h/cpp`：`Client : public QObject`，全局 `QPointer<Client> ClientInstance`（注释明确不应是单例，当前仍是）。约 50 个信号。`getPlayers()` 已 `Q_INVOKABLE`。`addPlayer`/`arrangeSeats`/`removePlayer` 玩家生命周期。
- `src/client/clientplayer.h`：`ClientPlayer : public Player`，全局 `QPointer<ClientPlayer> Self`。
- `src/core/player.h`：`Player` Q_PROPERTY 暴露 `seat`/`hp`/`renhp`/`linghp`/`maxhp`/`kingdom`/`role`/`general`/`general2`/`phase`/`alive`/`chained`/`avatar` 等。**`seat` 无 NOTIFY**；`avatar` MEMBER `m_avatar` NOTIFY `avatar_changed`；`phase`(QString, STORED false) 与 `phaseValue`(`Player::Phase` 枚举) 双属性。
- `src/core/protocol.h`：`QSanProtocol::CommandType` 枚举、`Countdown`。
- `src/core/util.h`：`IntList2VariantList` 等通用转换（桥接复用）。
- `src/uibackup/`：35 对死代码，不在 .pro（不编译），仅参考。

### QML 侧
- `qml/main.qml`：`Image` 背景 + `scalableRoot`（固定高 1440，宽随高缩放，最小 1920）+ `RootItem`。有"宽度过小提示"TODO。
- `qml/RootItem.qml`：`currentScene` 在 StartScene/RoomScene 间切换，监听 `MainWindowInstance.qml_switchToRoomScene`/`qml_switchToStartScene`。
- `qml/RoomScene.qml`：根 `CppRoomScene`。`property list<Photo> otherPhotos`（QTBUG-147713）。`lay()` 按 `effectiveSeat` 布局。`Component.onCompleted` 预创建占位 Photo（seat 2..N，未绑 player）。`Connections` 接收全部 `notify*`。`activeBox` 跟踪当前活动响应 box（按 status 自适应，同一时间只有一个）。`signal spaceClicked` + 根 MouseArea 空白点击触发（`RoleComboBox`/`HegRoleComboBox` 各自内部 `Connections { target: roomScene }` 监听以收起展开）。Dashboard 实例传 `photo: selfPhoto`。含 `testItemToBeRemovedAfterTest` 测试桩（`visible: !gameStarted`）。addRobot/fillRobots 已实现；未开始且未结算时显示"返回主菜单"按钮 `startSceneButton`（调 `MainWindowInstance.gotoStartScene()`）。
- `qml/Photo.qml`：`player` + `required property int seat` + `required property bool selfPhoto`（RoomScene 显式传 true/false：selfPhoto 实例 true，占位/测试 Photo false）。提取 `getGeneralName(g)`/`getImageSourceUrl(g)`（含 `_hegemony` 后缀处理）；source/visible 绑定；general2Image 对称 kingdom frame；player null fallback。`phase` 绑定 `player.phaseValue`（枚举）；未开始时 `general` 用 `player.avatar`；`PhaseItem` 用 `createPhaseItem()` 命令式创建（`Component.createObject`，`Component.onCompleted` 里 `if (!selfPhoto)` 调用——selfPhoto 不构造，用 Dashboard 的；无销毁；`visible` 绑 `gameStarted`；Component 模板内不访问外层 id 避开 `pragma ComponentBehavior: Bound` 告警）；`duozhi`（夺志，禁止角色使用/打出牌）时主副将图显示嘤嘤怪。化身图 `huashenImage`/`huashen2Image` source/visible 声明式绑定（`getImageSourceUrl(huashenGeneral)` / `huashenGeneral != ""`），`_hegemony` 由 `getImageSourceUrl` 统一 fallback；原 `onHuashenGeneralChanged`/`onHuashenGeneral2Changed` 命令式处理器（含手写 `_hegemony` 截断）已删除；opacity 循环动画（500ms 淡入→4s 停→500ms 淡出→1s 间歇）由 `onVisibleChanged` 启停。
- `qml/Dashboard.qml`：Trust/Discard/Cancel/OK 四按钮（`anchors.bottom: cardArea.top` 浮在手牌区上方）；`clientInstance` + `required property var photo`（绑 selfPhoto）属性。`updateStatus()` 按 `Client::status` switch 分支命令式设 `okEnabled`/`cancelEnabled`/`discardEnabled`（`Connections` 监听 status/refusable/activeBox/canAccept）；底部 `PhaseItem` 显示 `photo.phase`。
- `qml/CardItem.qml`：`signal clicked`/`rightClicked`（左/右键分发）；`selected` 属性注释掉（待实现）。
- `qml/GraphicsBox.qml`：图片背景可拖拽容器基类（Image 根，无标题/操作按钮）。
- `qml/ChooseGeneralBox.qml`：基于 GraphicsBox 的选将弹窗。

### 构建
- `QSanguosha.pro`：`SOURCES`/`HEADERS` 含 `src/qmlui/*`，`OTHER_FILES` 列 `qml/*.qml`。`src/uibackup` 未引用。
- `compile_commands.json`：构建目录 `/Users/fs/build-QSanguosha-Qt_6-Release`，clang++ Qt6 arm64 macOS。

## 3. 任务清单

### 已完成
- [x] **C++→QML 通知桥接层**：`connectClientSignals()` + 约 50 个 `notify*` 信号；`Q_PROPERTY` Self/ClientInstance 用具体类型；`Q_INVOKABLE replyToServer/notifyServer` 回传。
- [x] **RoomScene 基础功能**：notifyPlayerAdded（找空 Photo 绑 player）/notifySeatsArranged（读 player.seat 回填）/notifyPlayerRemoved（按 objectName 清空）；addRobot/fillRobots 按钮。
- [x] **Dashboard 操作按钮**：Trust 完整；OK 确认选将；Cancel/Discard 占位待卡牌选择。
- [x] **GraphicsBox 基类**：Image 根图片背景、可拖拽、default property content。
- [x] **ChooseGeneralBox**：单将/双将选择（singleResult）；右键 freechoose 换将（C++ FreeChooseDialog）；Dashboard OK 确认；回传 `name` 或 `name1+name2`。
- [x] **Photo.qml 重构**：getGeneralName/getImageSourceUrl 提取；_hegemony 后缀处理；general2Image 对称；null fallback。
- [x] **getUrl→getAssetUrl 重命名**：qml 侧 14 文件批量替换；qmlui.h 新增 assetExists。
- [x] **MainWindow 单例**：`QPointer<MainWindow> MainWindowInstance`（仿 RoomSceneInstance）。
- [x] **Client status 规范化**：`Q_PROPERTY ... NOTIFY status_changed`；`status_changed` 单参；`setStatus` 仅变化时 emit。
- [x] **assign_asked 身份分配**：桥接 `showRoleAssignDialog()` 弹模态 `RoleAssignDialog`（自包含：accept 调 `onPlayerAssignRole`、reject 调 `replyToServer`，无需 QML 返回值）；QML `onNotifyAssignAsked` 触发。
- [x] **国战双将势力校验**：`ChooseGeneralBox._canPair`（同势力或至少一方 `"zhu"`，对齐 `room.cpp:3723`/旧版 `choosegeneralbox.cpp:489`）+ `Engine::getGeneral` 加 `Q_INVOKABLE`（QML 直调 `Sanguosha.getGeneral(name).kingdom`，含 `_hegemony` fallback）；`_toggle` 拦截不合规第二将 + 不合规候选 opacity 灰显。修复国战可选不同势力的问题。

### 待做（按优先级）
- [ ] **CardItem 卡牌选择（打通 OK/Cancel/Discard 全链路）**：参考旧版 `src/uibackup/roomscene.cpp` 的 `useSelectedCard()`(2479)/`doOkButton()`(3083)/`doCancelButton()`(3100)/`doDiscardButton()`(3193)/`updateStatus()`(2743) 与 `src/uibackup/dashboard.cpp` 的 `getSelected`/`pendingCard`/`startPending`/`unselectAll`/`enableCards`。整条链路依赖较多，拆为以下子任务（按依赖顺序）：

  - [ ] **A. 牌区同步**（前置，覆盖所有玩家可见牌区）：`Client::getCards`/`loseCards` 回调对应 `move_cards_got`/`move_cards_lost` 信号（桥接**尚未转发**），按 `CardsMoveStruct.to_place`/`from_place`（`Player::Place` 枚举：`PlaceHand`/`PlaceEquip`/`PlaceDelayedTrick`/`PlaceJudge`/`PlaceSpecial`/`PlaceTable`/`DiscardPile`）与 `from`/`to` player 分发到对应区域。不仅是手牌：
    - **手牌区**（`PlaceHand`）✅：Dashboard `cardArea`（CardContainer）增删 CardItem 并 `lay()`。其他玩家手牌对 self 不可见，只同步 `ClientPlayer.handcard` 数量（已有 `Q_PROPERTY` + `handcardChanged`，Photo 已绑）。**其他玩家手牌移动过程中 `cardId` 可能为 -1（故意设计，客户端数据不对等——隐藏其他玩家手牌信息）**，CardItem 需处理 `cardId=-1`（显示牌背 `image/system/card-back.png`，不显示卡面/花色/点数）；`qml/CardItem.qml:onCardIdChanged` 已处理 `cardId=-1` 显示牌背。self 自己的手牌 `cardId` 是真实值，正常显示卡面。
    - **装备区**（`PlaceEquip`）：Dashboard `weaponArea`/`armorArea`/`dhorseArea`/`ohorseArea`/`treasureArea`（`qml/Dashboard.qml` 已有占位，当前 `visible: false`，需启用绑定）；其他玩家装备显示在 Photo 上。5 槽位（武器/防具/防马/攻马/宝物）按 `EquipCard::position()` 归位。
    - **判定区**（`PlaceDelayedTrick`）：放延时锦囊（闪电、乐不思蜀、兵粮寸断等）。self 在 Dashboard、其他在 Photo。需新建判定区容器组件。
    - **TablePile（桌面牌堆，统一处理 `PlaceJudge`/`PlaceTable`/`DiscardPile`，不区分）**：判定牌（`PlaceJudge`，判定过程翻出的实体牌，分出此 Place 是为给"红颜"等修改实体判定牌的技能留位置）、桌面打出的牌（`PlaceTable`）、弃牌堆（`DiscardPile`）三者都由 TablePile 显示，不做区分。TablePile 直接用 CardContainer（见"CardItem 与牌容器设计"小节）。**弃牌不直接 destroy，走延迟清除 + 重排机制**（见"CardItem 与牌容器设计"小节的"TablePile 延迟清除机制"）；`PlaceTable`/`PlaceJudge` → `DiscardPile` 是 TablePile 内部 place 变化，目前无专门动画。
    - **私人牌堆**（`PlaceSpecial`，如邓艾"屯田"的田、木牛流马 `wooden_ox`）：`ClientPlayer::getPileNames()` 动态获取 pile 名，`pile_changed(name)` 信号通知增删（`changePile`/`setPile` 触发）；显示在角色旁，可点击展开查看。需新建 pile 容器组件。
    - **公共牌区 / 五谷丰登（AG）**：由 `notifyAgFilled`/`notifyAgTaken`/`notifyAgCleared` 驱动（**已桥接**，见 `roomscene.h`），**不走 `move_cards_got/lost`**，AG 区不属于任何 player 的 place。
      - **旧代码 `PlaceWuGu` 情况**：旧版 `src/uibackup/roomscene.cpp:3902` 的 `takeAmazingGrace()` 用 `Player::PlaceWuGu` 作 `CardsMoveStruct.from_place` 标记牌来源（五谷丰登公共堆），**仅用于客户端取牌动画**；**当前 `src/core/player.h` 的 `Place` 枚举已删除 `PlaceWuGu`**（旧 uibackup 死代码引用了已不存在的枚举值，服务器侧也从不发送 PlaceWuGu，五谷丰登走 AG 机制 `S_COMMAND_FILL_AMAZING_GRACE`/`S_COMMAND_TAKE_AMAZING_GRACE`，对应 `ag_filled`/`ag_taken` 信号）。
      - **本次重构不引入 `PlaceWuGu` 即可保持功能**：`notifyAgTaken(taker, cardId, moveCards)` 已携带取牌动画所需的全部信息——`moveCards=true` 时 taker 手牌区增牌（走手牌同步，`to_place=PlaceHand`），取牌动画起点用 AG box 自身牌位置（QML 侧已知），无需专门的 `from_place` 枚举值；`moveCards=false` 时仅 AG 区移除该牌。AG box 是独立组件（待做，归入"玩家牌展示/桌面牌堆"任务的 PlayerCardBox/GenericCardContainer 范畴），A 子任务只需保证 `notifyAgTaken` 能驱动 taker 手牌区增删即可。
    - 实现要点：补桥接 `notifyMoveCardsGot`/`notifyMoveCardsLost`（转发 `move_cards_got`/`move_cards_lost`）。**`CardsMoveStruct` 在 C++ 桥接层处理，不暴露给 QML**（不 `qmlRegisterUncreatableType`）——桥接层解析每个 move，转成 QML 可理解的字段传给 QML：`cardIds` (QVariantList&lt;int&gt;)、`fromPlace`/`toPlace` (int，`Player::Place` 枚举值，`Q_ENUM` 已可用，QML 用 `Player.PlaceHand` 等比较)、`fromPlayer`/`toPlayer` (`Player *`，桥接层用结构体已有的 `from_player_name`/`to_player_name` 调 `ClientInstance.getPlayer(name)` 查得；`Player` 已 `qmlRegisterUncreatableType` 注册，QML 可直接读其 Q_PROPERTY)、`fromPileName`/`toPileName` (QString) 等。QML 侧 `onNotifyMoveCardsGot/Lost` 遍历 moves，按 `toPlace`/`fromPlace` + player（self vs 其他 Photo）分发到对应区域，增删后 `lay()`。
    - **`CardsMoveStruct` 的 `from`/`to` 指针**：`tryParse`(`src/core/structs.cpp:9`) 只解析 `from_player_name`/`to_player_name` 字符串，不解析 `from`/`to` 指针；但 `Client::getCards`/`loseCards`(`src/client/client.cpp:553-554`) 在 tryParse 后补设 `move.from = getPlayer(from_player_name)` / `move.to = getPlayer(to_player_name)`，所以 `move_cards_got`/`move_cards_lost` 信号触发时 moves 里的 `from`/`to` **非 null**（是 `ClientPlayer *`，也是 `Player *`）。桥接层直接用 `move.from`/`move.to` 作 `fromPlayer`/`toPlayer` 传给 QML（`Player` 已注册，QML 可读其 Q_PROPERTY），无需再 getPlayer。结构体 `from`/`to` 的 `Player *` 虚基类 dynamic_cast 问题（`src/core/structs.h:328`）后续细查。
    - **`isLastHandcard` 不传**：`tryParse` 未解析该字段（客户端恒为构造默认 false），客户端无使用，桥接层不传给 QML。
    - **不用 `CardsMoveStruct::toVariant()`**：该函数（`src/core/structs.cpp:37`）是服务器序列化用，输出 `JsonArray`（服务器协议格式，非 QML 语义化字段），且 `structs.cpp` `#include "room.h"` 可能有依赖。查证 `toVariant` 本身不直接用 `ServerPlayer`（只读 `card_ids`/`shown_ids`/`from_place`/`to_place`/`from_player_name`/`to_player_name`/`from_pile_name`/`to_pile_name`/`reason`/`open`），但格式不适合直接给 QML。桥接层自己解析这些字段转 QML 可理解内容。
    - **CardItem 与牌容器设计（前置）**：见"设计决策与约定"的"CardItem 与牌容器设计"小节——CardContainer 作为通用牌容器（TablePile 直接用，AG box 等 = GraphicsBox 内嵌 CardContainer）；CardItem 的 QObject parent 统一在 roomScene，移动用原实例（visual parent 切换：container → roomScene 飞行 → 新 container），不显示的牌直接 destroy。**`qml/CardContainer.qml:10` 的 `createItem` 现有 `createObject(this)` 必须改为 `createObject(roomScene)` + `cardItem.parent = this`**（否则牌移走后 QObject parent 还在 CardContainer，涉及转移问题）。
    - 无牌区同步则无从选卡/选装备，是 B-E 的前置。
  - [ ] **B. CardItem 选中态**：启用 `qml/CardItem.qml` 注释掉的 `selected` 属性 + `selectedChanged` 信号；单击 toggle `selected`；`enabled` 已联动禁用遮罩（见 2026-07-19 进度）。选中视觉对标旧版 `Dashboard::selectCard`(`uibackup/dashboard.cpp:538`)：
    - **抬起**：对标 `S_PENDING_OFFSET_Y = -25`(`dashboard.h:128`)。用 `transform: Translate { y: selected ? -25 : 0 }` + `Behavior` 动画，**不动 `y`/`homeY`**，避免与 `lay()`/`goBack()` 冲突（旧版改 homePos 会与 lay 重设打架，QML 用 transform 隔离更干净）。手牌区用。
    - **不使用 glow**：旧版 `QGraphicsDropShadowEffect`(`carditem.cpp:373`，Qt 自带 `QGraphicsEffect` 子类) 是 **hover 触发**（`hoverChanged → setEnabled`），非 selected 触发——对"选中时有 glow"的印象错误，故不引入 glow（不引入 `MultiEffect`/`QtQuick.Effects`；不接受 `Qt5Compat`）。
    - **ChooseGeneralBox 改造**：去掉 `qml/ChooseGeneralBox.qml:99` 的 `scale: ... ? 1.1 : 1.0`，改为 `selected: selectedGenerals.indexOf(modelData) >= 0`。ChooseGeneralBox 的 GridView `clip: true`，抬起 25 像素会被裁剪，故改用 **border 高亮**（selected 时 CardItem 加金色/白色 border）替换 scale 1.1，与手牌区抬起不同的 selected 视觉（按上下文）。
    - `qml/CardContainer.qml` 暴露 `selectedItems`（只读）/ `unselectAll(except)` / `selectOnlyCard()`。
  - [ ] **C. Dashboard pending 状态机 + PromptBox**：扩展 `Dashboard.updateStatus()` 按 status 进入 pending（镜像旧 `updateStatus` 各 case）：`Playing` → `enableCards`；`Responding`/`Discarding`/`Exchanging`/`AskForShowOrPindian` → `startPending(skill)` 按当前 pattern。手牌按 ViewAsSkill `viewFilter` enable/disable。pattern 来源 `Sanguosha->currentRoomState()->getCurrentCardUsePattern()`，需在 C++ 侧（`RoomState` 或桥接）暴露 `Q_PROPERTY`/`Q_INVOKABLE` 给 QML。新建 `qml/PromptBox.qml` 显示 prompt 文本（`Client::getPromptDoc()` 是 `QTextDocument*`，需桥接转 QString，参考旧 `roomscene.cpp:320`），随 status `appear`/`disappear`。
  - [ ] **D. 桥接选卡回传**：`const Card *` 无法跨 QML，桥接层 `RoomScene` 新增 `Q_INVOKABLE` 回传接口（如 `submitCardResponse(int status, QVariantList cardIds, QStringList targetNames)` / `submitDiscard(QVariantList cardIds)` / `cancelResponse(int status)`）。内部按 `cardIds` 经 `Client::getCard` 取 `const Card *`，`targetNames` 经 `Client::getPlayer` 转 `const Player *`，调 `Client::onPlayerResponseCard`/`onPlayerDiscardCards`/`onPlayerInvokeSkill` 等。**第一阶段不做 ViewAsSkill 组合牌**（如"龙鳞"用两牌当杀），只回传单张原始牌；组牌留待 G 子任务。
  - [ ] **E. OK/Cancel/Discard 按钮回传**：Dashboard 三按钮 `onClicked` 按 `clientInstance.status` 分支调桥接：OK → `submitCardResponse`/`submitDiscard`/`onPlayerInvokeSkill(true)`（AskForSkillInvoke）/`activeBox.accept()`（AskForGeneralTaken）；Cancel → `cancelResponse(status)`（`Playing`=unselectAll 不回传、`Responding`/`AskForShowOrPindian`=`onPlayerResponseCard(nullptr)`、`Discarding`/`Exchanging`=`onPlayerDiscardCards(nullptr)`、`ExecDialog`=reject dialog、`AskForSkillInvoke`=`onPlayerInvokeSkill(false)`、`AskForPlayerChoose`=`onPlayerChoosePlayer(nullptr)`）；Discard → `Playing` 状态 `onPlayerResponseCard(nullptr)`（结束出牌阶段）。
  - [ ] **F. 目标选择**：Photo 单击进入目标选择（status=`Playing`/`Responding` 且当前选中卡需目标，按 `card.targetFixed(Self)` 判断）；收集 `selected_targets`；Photo 视觉高亮可选目标（按 `card.targetFilter`，桥接 `Q_INVOKABLE enabledTargets(cardId)` 返回可选 player objectName 列表）；选满后 OK 启用。
  - [ ] **G. 技能按钮触发的 ViewAsSkill（与"Dashboard 技能按钮"任务交叉，合并到该任务）**：`QSanSkillButton` 点击 → `startPending(skill)`，组牌回传。依赖技能按钮组件。

  **简化策略（第一阶段最小闭环）**：A → B → C → D → E（先只支持 `targetFixed` 的卡：桃自用、无懈可击等），再 F 补目标选择（杀/闪/桃救人等），G 与"Dashboard 技能按钮"任务合并。
- [ ] **选项/触发顺序弹窗**：ChooseOptionsBox（askForChoice/askForOrder/askForDirection/askForSuit/askForKingdom）、ChooseTriggerOrderBox（askForTriggerOrder）。
- [ ] **玩家牌展示/桌面牌堆**：PlayerCardBox（showAllCards/showCard/askForGongxin）、GenericCardContainer、TablePile（askForGuanxing/askForYiji）。
- [ ] **聊天与日志**：ChatWidget、BubbleChatBox、ClientLogBox。
- [ ] **Dashboard 技能按钮**：`QSanSkillButton.qml` + 装备区绑定。
- [ ] **选将扩展**：askForGeneral3v3、askForRole3v3；KnownBoth（知己知彼卡牌效果，非国战双将）。
- [ ] **RoomScene 收尾**：移除 `testItemToBeRemovedAfterTest`、main.qml 宽度过小提示。
- [ ] **清理**：`src/uibackup` 死代码整体删除。
- [ ] **Client 去单例化**（QML 重构完成后着手，独立阶段）：`client.h` 末尾 TODO 注释明确"Client should ABSOLUTELY NOT be a singleton"——当前 `extern QPointer<Client> ClientInstance` 全局指针导致无法实现客户端侧 AI agent（只能服务端 AI）。改造方向：通过参数/上下文传入 Client 引用，移除全局 `ClientInstance`。**桥接层已部分铺垫**：`RoomScene::selfHelper()`/`clientHelper()`（注释"needed to refactor Self and ClientInstance from singleton"）当前返回 `Self`/`ClientInstance`，是去单例化的注入点；`clientHelper` 内亦有 TODO"consider how to get this after Client is no longer global singleton"。**注意自包含 dialog**：`RoleAssignDialog` 等 dialog 内部硬编码 `ClientInstance`（见"桥接架构"小节），需一并改造。

## 4. 设计决策与约定（讨论沉淀，后续必须沿用）

### 桥接架构
- **桥接集中在 CppRoomScene**：Client 信号统一在 `connectClientSignals()` 连接，`emit notify*` 转发 QML；QML 不直接依赖 Client 单例。符合"去单例化"方向（Client.h 注释明确不应是单例）。
- **notify 信号用具体类型**：参数用 `ClientPlayer *` 而非 `QObject *`（QML 类型提示好）；`Q_PROPERTY` 的 `Self`/`ClientInstance` 同理。
- **player 维护在 Client**：QML 不 accumulate 玩家副本，通过 `ClientInstance.getPlayers()`（已 Q_INVOKABLE）查询消费。
- **回传方式**：通用回传用 `replyToServer(int commandType, QVariant)`；简单请求直接调 Client slot（如 `addRobot()`/`trust()`/`onPlayerChooseGeneral(name)`）。
- **自包含 dialog（Client 去单例化时需处理）**：`RoleAssignDialog` 内部 accept() 直接调 `ClientInstance->onPlayerAssignRole(names, roles)`、reject() 调 `replyToServer(S_COMMAND_CHOOSE_ROLE, QVariant())` 自行回传服务器，因此桥接 `showRoleAssignDialog()` 返回 void、QML 无需返回值或后续处理。这与 `FreeChooseDialog`（桥接捕获 `general_chosen` 信号返回给 QML）模式不同。**Client 去单例化时**：此类 dialog 内部硬编码 `ClientInstance` 全局指针，必须改造为通过参数/上下文传入 Client 引用，否则会破坏；桥接层届时可考虑统一收集这类 dialog 的回传路径。
- **Engine 函数暴露给 QML**：遇到 Engine（`Sanguosha` 全局单例，已暴露给 QML）中 QML 需要调用但不可调用的函数，**直接在 Engine 类上加 `Q_INVOKABLE` 或 `Q_SLOT`，不走 RoomScene 桥接层**。判定：纯查询/无副作用/不需信号连接的加 `Q_INVOKABLE`（如 `getGeneral`）；需要被信号连接或有槽语义的加 `Q_SLOT`。Engine 已是 QML 可直接访问的全局对象，加宏后 QML 即可调用；返回的 QObject 子类需 `qmlRegisterUncreatableType` 注册后 QML/qmllint 才能识别其类型并读 `Q_PROPERTY`（`General` 已注册，可读 `kingdom`/`maxhp`/`gender`/`lord` 等）。

- **notify 处理函数的 log 约定**：`RoomScene.qml` 的 `onNotifyXxx` 占位时只放 `console.log` 作待实现标记；一旦为该 notify 添加了实际动作（调桥接/创建组件/遍历 Photo 等），**顺手删除其 `console.log`**（动作本身已表明信号到达，log 冗余）。

### Qt6 moc / QML 约束
- **Q_PROPERTY 指针类型需完整定义**：`Q_PROPERTY(T *)` 中 `T` 必须 include 完整定义，不能前置声明（否则 moc `static_assert(is_complete<...>)` 失败）。`roomscene.h` 需 `#include "client.h"`/`"clientplayer.h"`。
- **`Player.seat` 无 NOTIFY**：seat 变化 QML 无法自动感知，必须在 `notifySeatsArranged` 显式读 `photo.player.seat` 回填 `photo.seat`。
- **NOTIFY 信号规范**：无参或单参（新值）；仅值变化时 emit。`status_changed(Status newStatus)` 单参，`setStatus` 保留 `old_status` 仅 `old != new` 时 emit。
- **避免 const_cast**：信号参数需非 const 传 QML 时，直接改 Client 信号签名去 const（如 `cards_got`/`skill_acquired`）。
- **`QList<int>` 转换**：复用 `util.h::IntList2VariantList`，勿手写循环。

### 选将流程约定
- **不用 ExecDialog**：`askForGeneral` 统一 `setStatus(AskForGeneralTaken)`（原非国战走 ExecDialog 已改）。
- **OK 按钮复用 Dashboard 的**：响应 box（ChooseGeneralBox 等）不自带 OK，通过 `roomScene.activeBox`（通用，按 status 自适应，同一时间只有一个）跟踪，Dashboard OK 按钮触发 `activeBox.accept()`。
- **single_result 语义**：非国战/平异 = `true`（单将）；国战双将 = 服务器给定。`askForGeneral` 非国战分支需设 `single_result = true`（原保持 false 导致误走双将）。
- **国战双将势力校验**：双将必须同势力（kingdom），或至少一方为 `"zhu"`（百搭势力）。规则与服务器 `room.cpp:3723`、旧版 `uibackup/choosegeneralbox.cpp:489` 一致。QML 侧 `ChooseGeneralBox._canPair(g1, g2)` 直接调 `Sanguosha.getGeneral(name).kingdom`（`Engine::getGeneral` 已加 `Q_INVOKABLE`，QML 可直查，不判空不过度防御）；`_toggle` 选第二个将时拦截不合规搭配。**灰显**：`_isDimmed(g)` 联动 CardItem 标准 `enabled` 属性（`enabled: !_isDimmed(g)`），禁用时半透明黑遮罩覆盖（`visible: !enabled`，不用 opacity 避免漏 GraphicsBox 背景）——0 选不禁、1 选禁不可搭配、2 选禁全部未选。用 `enabled` 而非自定义属性，因 CardItem 作为手牌/装备等其他牌时也需禁用机制，统一复用。**OK 启用**：`canAccept`（单将≥1、双将=2）绑定 Dashboard OK 按钮 enabled 与 accept 校验，选未满不可确认。
- **回传格式**：单将 `name`，双将 `name1+name2`（与旧版 `reply()` 一致）。
- **右键 freechoose**：CardItem `rightClicked` 信号 + `ServerInfo.FreeChoose` → 调 `parent.freeChooseGeneral()`（C++ FreeChooseDialog modal exec）换将该位。
- **KnownBoth 是"知己知彼"卡牌效果**（非国战双将）；国战双将选择是独立需求。
- **国战将名 `_hegemony` 后缀是合法将名，`getGeneral` 查询不可去尾**：`Sanguosha.getGeneral("xxx_hegemony")` 直接查到国战将该本身；`xxx` 与 `xxx_hegemony` 是不同武将，kingdom 可能不同，去尾会查到错误的 general。注意这与 `Photo.qml` 的图片资源/翻译层面的 `_hegemony` 处理不同（图片资源可去尾 fallback 找文件、翻译可去尾查找 key，二者均非 getGeneral 查询）。

### UI 约定
- **GraphicsBox 基类**：Image 根（直接用 source 属性）、可拖拽、无标题/操作按钮/信号、default property content 槽位、Component.onCompleted 居中（x/y 而非 anchors，兼容拖拽）。
- **响应 box 接口约定**：所有需 Dashboard OK 确认的响应 box（ChooseGeneralBox/ChooseOptionsBox/ChooseTriggerOrderBox 等，多基于 GraphicsBox）须提供统一接口：`property bool canAccept`（当前选择是否可确认）+ `function accept()`（确认并回传/清理）。创建时 `roomScene.activeBox = box`，accept/销毁时 `roomScene.activeBox = null`。Dashboard 只认 `activeBox.canAccept`/`activeBox.accept()`，新增 box 类型不需改 Dashboard。同一时间只有一个活动 box（按 status 自适应）。
- **Dashboard 按钮**：`anchors.bottom: cardArea.top` + `bottomMargin` + `horizontalCenter` 浮在手牌区上方；268×133，font.pixelSize 50。**enabled 按 `Client::status`**：Discard=`Playing`；Cancel=`ExecDialog`/`AskForSkillInvoke` 或（`Responding`系列/`Discarding`/`Exchanging` 且 `discardActionRefusable`）；OK=活动 box `canAccept` 或 `AskForSkillInvoke`（其他响应状态待 CardItem 选卡落地后按选卡启用）。`Client::Status` 经 `Q_ENUM` + uncreatable 注册，QML 用 `Client.Playing` 等枚举名比较；`discardActionRefusable` 经 `Q_PROPERTY`（READ `isDiscardActionRefusable`/setter/NOTIFY `discardActionRefusableChanged`）暴露。enabled 由 `Dashboard.updateStatus()` 命令式设置（`okEnabled`/`cancelEnabled`/`discardEnabled` property），`updateStatus()`（switch case，含未来 prompt/card-pending/skill/target 复杂逻辑）只由 `status_changed` 调用；`onCanAcceptChanged` 直接设 `okEnabled = activeBox.canAccept`；不监听 `onDiscardActionRefusableChanged`（cancel 由 updateStatus 在 status 变化时重算）；`Component.onCompleted`/`onClientInstanceChanged` 直接置三按钮 false（等 `status_changed` 触发 updateStatus）。activeBox 创建/销毁不触发。。`updateStatus` 是 status 变化统一入口（镜像旧 `RoomScene::updateStatus`），后续 prompt/card pending/skill/target 选择等逻辑在此补；OK 点击在响应状态走目标选择等复杂流程，不只靠绑定。
- **资源访问**：统一用 `G.getAssetUrl(path)`（原 getUrl 已删）。
- **禁用 `z` 属性**：所有 UI 界面靠元素的声明顺序/父子层级（后声明的同级元素渲染在上层）解决相互覆盖，**不使用 `z` 属性**调整堆叠。**历史原因**：旧代码（`src/uibackup/`）滥用 `z` 调整堆叠，目前已用到小数点前 5 位（万级），为给以后调整留空间，现阶段能不用 `z` 就一律不用。
- **selfPhoto vs 非 selfPhoto 构造模式**：selfPhoto 与非 selfPhoto 共用 `Photo.qml` 组件，通过 `required property bool selfPhoto` 区分（RoomScene 显式传：selfPhoto 实例 `true`，占位/测试 Photo `false`）。**selfPhoto 不构造的子组件，由 Dashboard 承担显示**；非 selfPhoto 用 `Component.createObject` 命令式**动态构造**（`createPhaseItem()` 在 `Component.onCompleted` 调用一次创建，无销毁；`visible` 绑条件控制显隐；Component 模板内不访问外层 id，避开 `Loader` 的 `pragma ComponentBehavior: Bound` 告警）。已应用：`PhaseItem`（selfPhoto 不构造，用 Dashboard 的；非 selfPhoto 在 `Component.onCompleted` 里 `if (!selfPhoto) createPhaseItem()` 创建，`visible` 绑 `gameStarted`）。**后续装备区等同理**：selfPhoto 装备区在 Dashboard 构造（`Dashboard.qml` 已有 `weaponArea` 等占位），非 selfPhoto 装备区在 Photo 用 `createObject` 动态构造。

### CardItem 与牌容器设计（架构决策）
- **CardContainer 作为通用牌容器**（代替旧版 `GenericCardContainer` 基类）：
  - 旧版继承体系：`GenericCardContainer`（基类，`src/uibackup/GenericCardContainerUI.h:24`）→ `PlayerCardContainer`（玩家牌容器，:64，含 C++ layout）→ `Dashboard`(self)/`Photo`(其他玩家)；`CardContainer`（AG/五谷丰登公共牌区 box，`cardcontainer.h:28`）也继承 `GenericCardContainer`。
  - **旧版 `PlayerCardContainer`（`GenericCardContainerUI.h:64`）因 layout 原因不再使用**（QML 不用 C++ layout 体系）。
  - `qml/CardContainer.qml` 作为通用牌容器，提供统一的 add/remove/lay/选中接口（QML 无继承，用共同 property/function 签名约定）。各功能容器是 CardContainer 实例或内嵌 CardContainer：
    - Dashboard 手牌区：CardContainer。
    - 判定区、私人牌堆：CardContainer。
    - **TablePile（桌面牌堆）：直接用 CardContainer**。
    - **AG box / PlayerCardBox / GuanxingBox 等弹窗：GraphicsBox 内嵌 CardContainer**（GraphicsBox 提供可拖拽背景容器，CardContainer 提供牌展示，对应旧版 C++ `CardContainer` 的 AG box 角色）。
    - 装备区：5 槽位定制（待定是否用 CardContainer 变体）。
- **CardItem 的 QObject parent 统一在 RoomScene，移动用原实例**：
  - **QObject parent = roomScene**（`CppRoomScene` 根，常驻）。CardItem 创建统一走 `cardItemComponent.createObject(roomScene, {...})`，QObject parent = roomScene。**这样不涉及对象所有权转移**，只涉及 visual parent（显示坐标）切换——这正是 QObject parent 设为 roomScene 的原因。
  - **visual parent 按需切换**：容器内时 `cardItem.parent = container`（visual）。
  - **容器间移动用原实例，直接设为目标容器**：牌从 A 移到 B，visual parent 直接 A → B（不经过 roomScene 中间态）。切 parent 前后用 `mapToItem`/`mapFromItem` 把牌当前位置映射到 B 坐标系作为动画起点；B 调 `lay()` 设牌的 `homeX`/`homeY`（目标位置），`cardItem.goBack()` 动画从起点飞到 `homeX`/`homeY`。**利用 CardContainer 已做好的 `lay()` 函数**，动画终点由 lay 给出。QObject parent 始终是 roomScene，保留实例连续性与选中态。
  - **TablePile 延迟清除机制（牌进弃牌堆不直接 destroy）**：牌进 TablePile（`PlaceTable`/`PlaceJudge`/`DiscardPile` 统一）后，参考旧版 `src/uibackup/TablePile.cpp`——加入可见牌列表（`m_visibleCards`），超过可见数（`m_numCardsVisible`）或被 `clear(true)` 标记时 `_markClearance` 设清除时间戳（`tablePileClearTimeStamp`）；`timerEvent` 定时检查超时（`S_CLEARANCE_DELAY_BUCKETS`）后 `_fadeOutCardsLocked` 淡出动画（opacity→0）+ `deleteLater`，期间 `adjustCards()`（对应 QML 的 `lay()`）重新排列。即弃牌**一段时间后 destroy 且重新排列 TablePile**，不直接 destroy，也不"飞出"。`PlaceTable`/`PlaceJudge` → `DiscardPile` 是 TablePile 内部 place 变化，目前无专门动画。QML 侧 TablePile（CardContainer）需实现等价的延迟清除定时器 + 淡出 + 重排逻辑。
  - **不需要 cardPool**：没有"空闲暂存"状态——容器间移动直接到目标容器（`lay()` + `goBack()`）；无目标的牌 `destroy()`。牌按需动态创建/销毁，非静态预创建。
  - 容器销毁时（`Component.onDestruction`）：把其持有的 CardItem visual parent 重置回 roomScene，由 roomScene 统一管理后续（继续移动或 destroy）。QObject parent 永远是 roomScene，容器销毁不动 QObject 生命周期，安全。
- **CardContainer.qml 现有 `createItem` 需改造（涉及转移问题）**：`qml/CardContainer.qml:10` 的 `createItem(cardId)` 当前用 `createObject(this)`（QObject parent = CardContainer），**牌移到其他容器时 QObject parent 还在 CardContainer，涉及转移问题**。必须改为 `createObject(roomScene)`（QObject parent = roomScene）+ `cardItem.parent = this`（visual parent = CardContainer）。这样原实例移动只切 visual parent，不动 QObject parent。
- **旧版对象移动模式沿用精神**：旧版 C++ `RoomScene::_m_cardsMoveStash`(`uibackup/roomscene.cpp:2080`) 的 `from_container->removeCardItems` → `to_container->addCardItems` 实例转移 + `setParentItem` 切 visual。QML 沿用"原实例移动"精神，用 `cardItem.parent = xxx`（QQuickItem visual parent）替代 `setParentItem`，QObject parent 固定 roomScene 替代 C++ 自由 reparent。

### 其他
- **qmllint 假告警**：未生成 qmltypes 时 `CppRoomScene`/`rocks.touhousatsu` 未识别，大量 `unqualified`/`missing-type` warning，构建后消除，非真实错误。
- **日志**：桥接层 `qDebug` 带 `[bridge]` 前缀，不打印大 payload。
- **旧代码类名注意**：`src/uibackup/` 中以 `Q+大写字母` 打头的类不全是 Qt 自带，有自定义类：`QSan*` 系列（`QSanSelectableItem`/`QSanButton`/`QSanSkillButton`/`QSanInvokeSkillButton` 等）与 `QAnimatedEffect`(`src/uibackup/sprite.h:28`，继承 Qt 自带 `QGraphicsEffect`，用于动画效果，配合 `EffectAnimation` 使用)。阅读代码时需确认是 Qt 自带（如 `QGraphicsDropShadowEffect`/`QGraphicsObject`/`QGraphicsProxyWidget`/`QGraphicsEffect`）还是自定义（`QSan*` 系列、`QAnimatedEffect`）。
- **旧代码宏定义注意**：`G_ROOM_SKIN`/`G_DASHBOARD_LAYOUT`/`G_ROOM_LAYOUT`/`G_PHOTO_LAYOUT`/`G_COMMON_LAYOUT` 等是宏定义（`#define`，在 `src/dialog/uilegacy/SkinBank.h:470-474`，**非全局变量**），旧 `uibackup` 代码大量使用（如 `G_COMMON_LAYOUT.m_cardNormalHeight`、`G_ROOM_LAYOUT.m_discardPilePadding` 等）。`SkinBank.h` 从 `uibackup` 移到了 `src/dialog/uilegacy/`，**仍在用**（非死代码）。阅读旧代码时 `G_*` 形式的标识符需确认是宏还是变量。
- **兼容性**：保留 `MainWindow` 现有 `qml_switchToRoomScene` 等接口签名。
- **提交前格式化**：C++ 全文件运行 clang-format，QML 全文件运行 qmlformat。**注意 qmlformat 会重排属性/函数先后顺序**，运行后需复查注释是否仍与对应代码位置对得上（如 property 上方的注释、函数前的注释）。`property list<Photo>` 等 qmlformat 不兼容的写法见 QTBUG-147713（RoomScene.qml 注释），需在 qmlformat 前临时移除、之后加回。

## 5. 关键代码流程

### 玩家生命周期
- `Client::addPlayer`（client.cpp:467）：解析 `[name, screen_name(base64), avatar]`，`new ClientPlayer`，`setObjectName`，`players <<`，`emit player_added`。**不设 seat**。
- `Client::arrangeSeats`（client.cpp:780）：按服务器 player_names 顺序 `players.clear()` 重建，`setSeat(i+1)`/`setNext`，`emit seats_arranged()`（无参）。seat 此时才设置。
- `Client::removePlayer`（client.cpp:502）：`setParent(nullptr)`，`emit player_removed(name)`，`players.removeOne`，`deleteLater` 绑 Client::destroyed。**player 不立即销毁**。

### QML 场景切换
`MainWindow::qml_switchToRoomScene` → `RootItem` Connections → `roomSceneComponent.createObject`。此时 ClientInstance/Self 已存在，RoomScene 构造立即 `connectClientSignals()`。

### 选将数据流
`Client::askForGeneral` emit `generals_got(generals, single_result, can_convert)` → 桥接 `notifyGeneralsGot` → RoomScene `onNotifyGeneralsGot` 创建 ChooseGeneralBox（传 singleResult，设 activeBox）→ 选将（右键可 freechoose）→ Dashboard OK → `accept()` emit `generalChosen` → `ClientInstance.onPlayerChooseGeneral(name)`（`replyToServer(S_COMMAND_CHOOSE_GENERAL)`）。

## 6. 进度记录

### 2026-07-18：C++→QML 通知通路
- `roomscene.h`：`connectClientSignals()` + 约 50 个 `notify*` 信号（QML 友好类型）；`Q_PROPERTY` Self/ClientInstance 改具体类型（需 include 完整定义满足 moc）；`Q_INVOKABLE replyToServer/notifyServer`。
- `roomscene.cpp`：lambda 集中连接 Client 全部信号；`QList<int>` 复用 `IntList2VariantList`；`[bridge]` 日志。
- `client.h`：`cards_got`/`skill_acquired` 去 const；`getPlayers()` 加 `Q_INVOKABLE`；`seats_arranged()` 删参数。
- `RoomScene.qml`：`Connections` 接收全部 `notify*`。

### 2026-07-18：RoomScene 基础功能
- notifyPlayerAdded/SeatsArranged/PlayerRemoved 实现。
- addRobot/fillRobots 按钮（visible 含 ServerInfo.EnableAI）。

### 2026-07-18：Dashboard 操作按钮
- Trust/Discard/Cancel/OK 四按钮，anchors 浮在手牌区上方。
- Trust 完整；OK 确认选将；Cancel/Discard 占位待卡牌选择。

### 2026-07-18：GraphicsBox 基类
- `qml/GraphicsBox.qml`：Image 根图片背景容器，可拖拽，default property content。

### 2026-07-18：ChooseGeneralBox 单将/双将选择
- 基于 GraphicsBox，GridView + CardItem，_toggle 选中（单将1/双将2），回传 `name` 或 `name1+name2`。
- 右键 freechoose 换将（C++ FreeChooseDialog）。
- Dashboard OK 触发 accept。
- `askForGeneral` 非国战 single_result=true、setStatus 统一 AskForGeneralTaken。
- `status_changed` 改单参规范，setStatus 仅变化时 emit。
- `freeChooseGeneral()` 桥接（parent 用 MainWindowInstance）。

### 2026-07-19：Photo.qml 重构
- 提取 getGeneralName/getImageSourceUrl（_hegemony 后缀处理）。
- source/visible 改绑定；general2Image 对称 kingdom frame；null fallback；maxhp 绑 player.maxhp。

### 2026-07-19：getUrl → getAssetUrl
- qmlui.h：getUrl 重命名为 getAssetUrl，新增 assetExists（getUrl 已删）。
- qml/*.qml：14 文件批量替换 G.getUrl → G.getAssetUrl。

### 2026-07-19：MainWindow 单例
- `QPointer<MainWindow> MainWindowInstance`（仿 RoomSceneInstance），构造函数第一行赋值。

### 2026-07-19：assign_asked 身份分配
- `roomscene.h`：新增 `Q_INVOKABLE void showRoleAssignDialog()` 声明（紧随 `freeChooseGeneral`）。
- `roomscene.cpp`：include `roleassigndialog.h`；实现 `showRoleAssignDialog()` —— 栈对象 `RoleAssignDialog dialog(MainWindowInstance); dialog.exec();`。RoleAssignDialog 自包含（accept 调 `ClientInstance->onPlayerAssignRole`、reject 调 `replyToServer(S_COMMAND_CHOOSE_ROLE, QVariant())`），无需 QML 返回值。
- `RoomScene.qml`：`onNotifyAssignAsked` 调用 `roomScene.showRoleAssignDialog()`。
- `roleassigndialog.cpp/.h` 已在 .pro（无需改构建）。

### 2026-07-19：国战双将势力校验
- 修复国战选将可选不同势力的问题：双将必须同势力或至少一方 `"zhu"`。
- `engine.h`：`Engine::getGeneral` 加 `Q_INVOKABLE`，QML 直接 `Sanguosha.getGeneral(name).kingdom` 读势力，不走 RoomScene 桥接。
- `ChooseGeneralBox.qml`：`_canPair(g1, g2)` 直接 `Sanguosha.getGeneral(name).kingdom`（对齐 `room.cpp:3723`/旧版 `choosegeneralbox.cpp:489`，不判空不过度防御）；`_toggle` 双将模式选第二个将时校验，不合规拒绝；不合规候选 `opacity` 0.3 灰显。
- 注：初版曾在桥接层加 `getGeneralKingdom`，后按约定改为直接给 Engine 加 `Q_INVOKABLE`（见"桥接架构"小节"Engine 函数暴露给 QML"规则）。初版 `_canPair` 曾经 `_kingdomOf` 加 `_hegemony` 去尾 fallback，已删除——国战将名带 `_hegemony` 后缀是合法将名，去尾会查到 kingdom 不同的另一个将（见"选将流程约定"小节）。

### 2026-07-19：注册 General 到 QML
- `qmlui.cpp`：`registerCore` 新增 `qmlRegisterUncreatableType<General>`（紧随 ClientPlayer），include `general.h`。
- 目的：`Engine::getGeneral` 加 `Q_INVOKABLE` 后 QML 拿到 `const General *`，注册 General 类型使 QML/qmllint 能识别并读其 `Q_PROPERTY`（`kingdom`/`maxhp`/`gender`/`lord`/`hidden` 等），后续可复用，不再依赖未注册类型的元对象回退。

### 2026-07-19：国战选将 UX 修复（3 项）
- **OK 过早启用**：ChooseGeneralBox 加 `canAccept`（单将≥1、双将=2），`accept()` 与 Dashboard OK 按钮 enabled 统一用它，双将选 1 个不可确认。
- **选满 2 个后其余应保持灰显**：新增 `_isDimmed(g)`（0 选不灰、1 选灰不可搭配、2 选灰全部未选），替代原 `opacity` 绑定（原条件仅 `length===1`，选满后恢复不灰）。
- **灰显漏背景**：CardItem 加 `dimmed` 属性 + 半透明黑色遮罩 Rectangle（靠声明顺序覆盖，不用 z/opacity），不漏 GraphicsBox 背景。

### 2026-07-19：确立"禁用 z 属性"约定
- UI 约定新增：所有 UI 靠声明顺序/父子层级解决覆盖，不用 `z`。
- CardItem 灰显遮罩移除 `z: 1`（靠在 cardContent/MouseArea 之后声明保证上层覆盖）。

### 2026-07-19：CardItem 禁用机制改用标准 enabled
- 灰显从自定义 `dimmed` 属性改为 Qt 标准 `enabled` 属性联动：`enabled: !_isDimmed(g)`，遮罩 `visible: !enabled`。
- 原因：CardItem 作为手牌/装备等其他牌时也需禁用情况，统一用 `enabled` 复用一套机制（禁用时 MouseArea 自动不响应 + 遮罩变暗）。

### 2026-07-19：gameStarted 状态同步给 Photo
- 问题：`Client::game_started` 信号到达时桥接只 `emit notifyGameStarted()`，未置 C++ `gameStarted` 成员，导致 `roomScene.gameStarted` 恒 false，Photo 的 `gameStarted` 绑定（`Qt.binding(() => roomScene.gameStarted)`）永不刷新，血条/手牌数/roleComboBox/kingdom frame 等不显示。
- 修复 `roomscene.cpp`：`game_started` lambda 里 `gameStarted = true; emit gameStartedChanged(true);`（`if (!gameStarted)` 幂等）。
- Photo 侧绑定早已就绪（otherPhotos `Qt.binding`、selfPhoto `gameStarted: roomScene.gameStarted`），无需改 QML。

### 2026-07-19：gameover/standoff 结算对话框
- 新建 `src/dialog/gameoverdialog.h/cpp`（从 `uibackup/roomscene.cpp` 的 onGameOver/onStandoff/fillTable 移植）：`GameOverDialog(standoff, parent)`，standoff=单表所有玩家 / 非standoff=胜负两表（按 `player->property("win")`）；`fillTable` 10 列统计（RecAnalysis 回放分析：回合数/伤害/受击/击杀/回血/最后手牌）；返回主菜单按钮 `accept()` + `QTimer::singleShot(0, MainWindowInstance, gotoStartScene)` 延迟切场景（避免 exec 栈帧与 RoomScene 销毁冲突）。
- 桥接 `RoomScene::showGameOverDialog(bool standoff)`（同 freeChoose/showRoleAssignDialog 模式，栈对象 exec）；`game_over`/`standoff` lambda 同步设 `gameOver=true` + `emit gameOverChanged`（同 gameStarted 同步）。
- `RoomScene.qml`：`onNotifyGameOver` 调 `showGameOverDialog(false)`、`onNotifyStandoff` 调 `showGameOverDialog(true)`。
- `.pro` 加 `gameoverdialog.cpp/.h`。
- TODO：重启游戏/保存录像按钮未接（旧代码 `RoomScene::restart()`/`saveReplayRecord`，mainwindow.cpp 已注释，待桥接）。

### 2026-07-19：Dashboard 按钮 enabled 按 Client::status 更新
- OK/Cancel/Discard 的 enabled 从硬编码 false 改为按 `dashboard.clientInstance.status` 绑定（`Client::Status` 经 `Q_ENUM` + uncreatable 注册，QML 用 `Client.Playing` 等枚举名比较）。
- Discard=`Playing`；Cancel=`ExecDialog`/`AskForSkillInvoke` 或（`Responding`系列/`Discarding`/`Exchanging` 且 `discardActionRefusable`）；OK=选将 `canAccept` ‖ `AskForSkillInvoke`。Responding 系列用 `status & Client.ClientStatusBasicMask === Client.Responding` 判断（涵盖 RespondingUse 等高位变体）。
- 暴露 `m_isDiscardActionRefusable` 给 QML：`client.h` 加 `Q_PROPERTY(bool discardActionRefusable READ isDiscardActionRefusable NOTIFY discardActionRefusableChanged)` + getter/setter + 信号（成员保留 public）；`client.cpp` 实现 setter（仅变化时 emit），9 处 `m_isDiscardActionRefusable = X` 改走 `setDiscardActionRefusable(X)`（构造函数初始化列表保留）。
- 按钮 enabled 改为 `Dashboard.updateStatus()` 命令式设置（`okEnabled`/`cancelEnabled`/`discardEnabled` property），`Connections` 监听 status/refusable/activeBox/canAccept 变化触发。`updateStatus` 用 `switch(status & ClientStatusBasicMask)` 按 status 分支设按钮 enabled（镜像旧 `uibackup/roomscene.cpp:2784-2949`），后续 prompt/card pending/skill/target 选择等逻辑在各 case 补；OK 点击在响应状态走目标选择等复杂流程，不只靠绑定。
- Responding 等状态的 OK 待 CardItem 选卡落地后按选卡启用。参考 `uibackup/roomscene.cpp:2784-2949`。

### 2026-07-20：A-手牌区同步实现
- **桥接层**：`roomscene.h` 加 `notifyMoveCardsGot`/`notifyMoveCardsLost` 信号（携 `moveId` + `QVariantList moves`）；`roomscene.cpp` `connectClientSignals()` 连接 `Client::move_cards_got`/`move_cards_lost`，lambda 遍历 `QList<CardsMoveStruct>` 转 `QVariantList<QVariantMap>`（字段：`cardIds`/`fromPlace`/`toPlace`/`fromPlayer`(`Player *`)/`toPlayer`(`Player *`)/`fromPileName`/`toPileName`），`from`/`to` 直接用结构体指针（`getCards`/`loseCards` 已 `getPlayer` 补设，非 null），加 `#include "structs.h"`。
- **CardContainer.qml**：加 `id: cardContainer` + `property var rootScene`；`createItem` 改 `createObject(rootScene)` + `item.parent = cardContainer`（QObject parent = roomScene，visual parent = cardContainer，见"CardItem 与牌容器设计"小节）；加 `removeItem(cardId)`（按 cardId 找 + destroy）。
- **Dashboard.qml**：加 `property var roomScene` + `addHandCard(cardId)`/`removeHandCard(cardId)` 函数（调 `cardArea.createItem`/`removeItem` + `lay(Qt.AlignLeft, 1, 0, true, true)`）；CardContainer 设 `rootScene: dashboard.roomScene`。
- **RoomScene.qml**：Dashboard 实例设 `roomScene: roomScene`；`onNotifyMoveCardsGot`/`onNotifyMoveCardsLost` 处理 `toPlace`/`fromPlace == Player.PlaceHand && player.objectName == Self.objectName` 的 move，遍历 `cardIds` 调 `dashboard.addHandCard`/`removeHandCard`。
- **CardItem.qml**：`onCardIdChanged` 的 `cardId == -1` 分支补全牌背显示（`cardImage.source = card-back.png` + 隐藏花色/点数），之前只 `return` 未设 source。
- 编译通过（roomscene.o + 链接成功）。qmllint 仅 1 条假告警（CardContainer.qml createObject 返回类型推断为 QObject，实际 CardItem）。

### 2026-07-20：PhaseItem 构造模式 + selfPhoto 标识 + QSanButton hover
- 确立 **selfPhoto vs 非 selfPhoto 构造模式**（见"UI 约定"小节）：selfPhoto 不构造的子组件由 Dashboard 承担显示，非 selfPhoto 用 `createObject` 动态构造。
- `qml/Photo.qml`：`selfPhoto` 改 `required property bool selfPhoto`（强制显式传值）；`PhaseItem` 从静态声明 + `visible: !selfPhoto` 改为 `createPhaseItem()` 命令式创建（`Component.createObject`，`Component.onCompleted` 里 `if (!selfPhoto)` 调用——selfPhoto 不构造；无销毁，`visible` 绑 `gameStarted`）——避开 `Loader` 的 `pragma ComponentBehavior: Bound` 告警（Component 模板 `PhaseItem {}` 内不访问外层 id，`Qt.binding(() => photo.xxx)` 闭包绑 anchors/phase/visible）。
- `qml/RoomScene.qml`：selfPhoto 实例设 `selfPhoto: true`；占位 Photo（`Component.onCompleted` 创建）与测试桩 Photo 显式 `selfPhoto: false`。
- `qml/QSanButton.qml`：hover 状态加 `hover.color`（`entered`/`downEntered`/`downExited` 用 `Qt.rgba(1,1,1,.25)` 白色半透明，`disabled` 用 `Qt.rgba(0,0,0,.25)` 黑色半透明）。
- 后续装备区同理：selfPhoto 在 Dashboard 构造，非 selfPhoto 在 Photo 用 `createObject` 动态构造。

### 2026-07-19：Photo 化身图绑定化 + RoomScene 测试桩显隐
- `Photo.qml`：化身图 `huashenImage`/`huashen2Image` 改声明式绑定（`source: getImageSourceUrl(huashenGeneral)`、`visible: huashenGeneral != ""`），`_hegemony` 后缀 fallback 由 `getImageSourceUrl` 内部 `assetExists` 统一处理；删除 `onHuashenGeneralChanged`/`onHuashenGeneral2Changed` 命令式处理器（含手写 `_hegemony` 截断逻辑，现由 `getImageSourceUrl` 覆盖）。化身图 opacity 循环动画（500ms 淡入→4s 停→500ms 淡出→1s 间歇）由 `onVisibleChanged` 启停。
- `RoomScene.qml`：`testItemToBeRemovedAfterTest` 的 `visible` 从 `true` 改为 `!gameStarted`（游戏开始后隐藏测试桩）。
- 补记：RoomScene 已有的"返回主菜单"按钮 `startSceneButton`（`visible: !gameStarted && !gameOver`，调 `MainWindowInstance.gotoStartScene()`），plan 之前遗漏。

## 7. 下一步
1. **CardItem 卡牌选择全链路**（已拆分为 A-G 子任务，见"待做"小节）：
   - 执行顺序：A（牌区同步：手牌/装备/判定/私人牌堆）→ B（CardItem 选中态）→ C（Dashboard pending + PromptBox）→ D（桥接选卡回传）→ E（OK/Cancel/Discard 按钮回传）→ F（目标选择）。
   - 第一阶段最小闭环：A+B+C+D+E（先只支持 `targetFixed` 卡：桃自用、无懈可击等），F 补目标选择（杀/闪/桃救人），G 与"Dashboard 技能按钮"任务合并。
2. 选项/触发顺序弹窗（ChooseOptionsBox/ChooseTriggerOrderBox）。
3. 玩家牌展示/桌面牌堆、聊天日志。
4. Dashboard 技能按钮（含 G 子任务：技能按钮触发的 ViewAsSkill 组牌）、RoomScene 测试桩清理、src/uibackup 删除。
5. QML 重构完成后：Client 去单例化（见 `client.h` TODO；桥接 `selfHelper`/`clientHelper` 已预留注入点，自包含 dialog 需一并改造）。
