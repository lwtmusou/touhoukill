# QML 重构计划 — touhoukill (qt6_ui 分支)

> 本文件是 QML 重构的主索引：项目结构、任务清单、设计约定、踩坑记录、进度。
> **维护指示：每次大幅度更新（新增功能模块/改动桥接层/完成一个 notify 处理或 UI 组件/重命名）后，自主更新本文件的"任务清单"状态、"进度记录"与"目录结构"，不等用户提醒。** 小幅格式调整不必记。

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
- `src/core/player.h`：`Player` Q_PROPERTY 暴露 `seat`/`hp`/`kingdom`/`role`/`general`/`phase`/`alive`/`chained` 等。**`seat` 无 NOTIFY**。
- `src/core/protocol.h`：`QSanProtocol::CommandType` 枚举、`Countdown`。
- `src/core/util.h`：`IntList2VariantList` 等通用转换（桥接复用）。
- `src/uibackup/`：35 对死代码，不在 .pro（不编译），仅参考。

### QML 侧
- `qml/main.qml`：`Image` 背景 + `scalableRoot`（固定高 1440，宽随高缩放，最小 1920）+ `RootItem`。有"宽度过小提示"TODO。
- `qml/RootItem.qml`：`currentScene` 在 StartScene/RoomScene 间切换，监听 `MainWindowInstance.qml_switchToRoomScene`/`qml_switchToStartScene`。
- `qml/RoomScene.qml`：根 `CppRoomScene`。`property list<Photo> otherPhotos`（QTBUG-147713）。`lay()` 按 `effectiveSeat` 布局。`Component.onCompleted` 预创建占位 Photo（seat 2..N，未绑 player）。`Connections` 接收全部 `notify*`。`activeChooseGeneralBox` 跟踪选将框。含 `testItemToBeRemovedAfterTest` 测试桩。addRobot/fillRobots 已实现。
- `qml/Photo.qml`：`player` + `required property int seat`。提取 `getGeneralName(g)`/`getImageSourceUrl(g)`（含 `_hegemony` 后缀处理）；source/visible 绑定；general2Image 对称 kingdom frame；player null fallback。
- `qml/Dashboard.qml`：Trust/Discard/Cancel/OK 四按钮（`anchors.bottom: cardArea.top` 浮在手牌区上方）；`clientInstance` 属性。
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
- [ ] **CardItem 卡牌选择**：启用 `selected` 属性 + 信号 → 打通 Dashboard OK/Cancel/Discard（useSelectedCard → onPlayerResponseCard/onPlayerDiscardCards）。
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
- **OK 按钮复用 Dashboard 的**：ChooseGeneralBox 不自带 OK，通过 `roomScene.activeChooseGeneralBox` 跟踪，Dashboard OK 按钮触发 `accept()`。
- **single_result 语义**：非国战/平异 = `true`（单将）；国战双将 = 服务器给定。`askForGeneral` 非国战分支需设 `single_result = true`（原保持 false 导致误走双将）。
- **国战双将势力校验**：双将必须同势力（kingdom），或至少一方为 `"zhu"`（百搭势力）。规则与服务器 `room.cpp:3723`、旧版 `uibackup/choosegeneralbox.cpp:489` 一致。QML 侧 `ChooseGeneralBox._canPair(g1, g2)` 直接调 `Sanguosha.getGeneral(name).kingdom`（`Engine::getGeneral` 已加 `Q_INVOKABLE`，QML 可直查，不判空不过度防御）；`_toggle` 选第二个将时拦截不合规搭配。**灰显**：`_isDimmed(g)` 联动 CardItem 标准 `enabled` 属性（`enabled: !_isDimmed(g)`），禁用时半透明黑遮罩覆盖（`visible: !enabled`，不用 opacity 避免漏 GraphicsBox 背景）——0 选不禁、1 选禁不可搭配、2 选禁全部未选。用 `enabled` 而非自定义属性，因 CardItem 作为手牌/装备等其他牌时也需禁用机制，统一复用。**OK 启用**：`canAccept`（单将≥1、双将=2）绑定 Dashboard OK 按钮 enabled 与 accept 校验，选未满不可确认。
- **回传格式**：单将 `name`，双将 `name1+name2`（与旧版 `reply()` 一致）。
- **右键 freechoose**：CardItem `rightClicked` 信号 + `ServerInfo.FreeChoose` → 调 `parent.freeChooseGeneral()`（C++ FreeChooseDialog modal exec）换将该位。
- **KnownBoth 是"知己知彼"卡牌效果**（非国战双将）；国战双将选择是独立需求。
- **国战将名 `_hegemony` 后缀是合法将名，`getGeneral` 查询不可去尾**：`Sanguosha.getGeneral("xxx_hegemony")` 直接查到国战将该本身；`xxx` 与 `xxx_hegemony` 是不同武将，kingdom 可能不同，去尾会查到错误的 general。注意这与 `Photo.qml` 的图片资源/翻译层面的 `_hegemony` 处理不同（图片资源可去尾 fallback 找文件、翻译可去尾查找 key，二者均非 getGeneral 查询）。

### UI 约定
- **GraphicsBox 基类**：Image 根（直接用 source 属性）、可拖拽、无标题/操作按钮/信号、default property content 槽位、Component.onCompleted 居中（x/y 而非 anchors，兼容拖拽）。
- **Dashboard 按钮**：`anchors.bottom: cardArea.top` + `bottomMargin` + `horizontalCenter` 浮在手牌区上方；268×133，font.pixelSize 50。**enabled 按 `Client::status`**：Discard=`Playing`；Cancel=`ExecDialog`/`AskForSkillInvoke` 或（`Responding`系列/`Discarding`/`Exchanging` 且 `discardActionRefusable`）；OK=选将 `canAccept` 或 `AskForSkillInvoke`（其他响应状态待 CardItem 选卡落地后按选卡启用）。`Client::Status` 经 `Q_ENUM` + uncreatable 注册，QML 用 `Client.Playing` 等枚举名比较；`discardActionRefusable` 经 `Q_PROPERTY`（READ `isDiscardActionRefusable`/setter/NOTIFY `discardActionRefusableChanged`）暴露。
- **资源访问**：统一用 `G.getAssetUrl(path)`（原 getUrl 已删）。
- **禁用 `z` 属性**：所有 UI 界面靠元素的声明顺序/父子层级（后声明的同级元素渲染在上层）解决相互覆盖，**不使用 `z` 属性**调整堆叠。**历史原因**：旧代码（`src/uibackup/`）滥用 `z` 调整堆叠，目前已用到小数点前 5 位（万级），为给以后调整留空间，现阶段能不用 `z` 就一律不用。

### 其他
- **qmllint 假告警**：未生成 qmltypes 时 `CppRoomScene`/`rocks.touhousatsu` 未识别，大量 `unqualified`/`missing-type` warning，构建后消除，非真实错误。
- **日志**：桥接层 `qDebug` 带 `[bridge]` 前缀，不打印大 payload。
- **兼容性**：保留 `MainWindow` 现有 `qml_switchToRoomScene` 等接口签名。

## 5. 关键代码流程

### 玩家生命周期
- `Client::addPlayer`（client.cpp:467）：解析 `[name, screen_name(base64), avatar]`，`new ClientPlayer`，`setObjectName`，`players <<`，`emit player_added`。**不设 seat**。
- `Client::arrangeSeats`（client.cpp:780）：按服务器 player_names 顺序 `players.clear()` 重建，`setSeat(i+1)`/`setNext`，`emit seats_arranged()`（无参）。seat 此时才设置。
- `Client::removePlayer`（client.cpp:502）：`setParent(nullptr)`，`emit player_removed(name)`，`players.removeOne`，`deleteLater` 绑 Client::destroyed。**player 不立即销毁**。

### QML 场景切换
`MainWindow::qml_switchToRoomScene` → `RootItem` Connections → `roomSceneComponent.createObject`。此时 ClientInstance/Self 已存在，RoomScene 构造立即 `connectClientSignals()`。

### 选将数据流
`Client::askForGeneral` emit `generals_got(generals, single_result, can_convert)` → 桥接 `notifyGeneralsGot` → RoomScene `onNotifyGeneralsGot` 创建 ChooseGeneralBox（传 singleResult，设 activeChooseGeneralBox）→ 选将（右键可 freechoose）→ Dashboard OK → `accept()` emit `generalChosen` → `ClientInstance.onPlayerChooseGeneral(name)`（`replyToServer(S_COMMAND_CHOOSE_GENERAL)`）。

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
- Responding 等状态的 OK 待 CardItem 选卡落地后按选卡启用。参考 `uibackup/roomscene.cpp:2784-2949`。

## 7. 下一步
1. CardItem 卡牌选择（`selected` 状态 + 信号）→ 打通 OK/Cancel/Discard。
2. 选项/触发顺序弹窗（ChooseOptionsBox/ChooseTriggerOrderBox）。
3. 玩家牌展示/桌面牌堆、聊天日志。
4. Dashboard 技能按钮、RoomScene 测试桩清理、src/uibackup 删除。
5. QML 重构完成后：Client 去单例化（见 `client.h` TODO；桥接 `selfHelper`/`clientHelper` 已预留注入点，自包含 dialog 需一并改造）。
