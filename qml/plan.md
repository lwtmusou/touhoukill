# QML 重构计划 — touhoukill (qt6_ui 分支)

> 本文件是 QML 重构的主索引：项目结构、任务清单、设计约定、踩坑记录、进度。
> **维护指示**：
> - 每次大幅度更新（新增功能模块/改动桥接层/完成一个 notify 处理或 UI 组件/重命名）后，自主更新本文件的"任务清单"状态、"进度记录"与"目录结构"，不等用户提醒。小幅格式调整不必记。
> - **每次修改文件后格式化**（含新增文件）：每完成一个 `.cpp`/`.h` 文件修改或新增后主动运行 clang-format 全文件格式化，每完成一个 `.qml` 文件修改或新增后主动运行 qmlformat 全文件格式化（不等提交）。注意 qmlformat 会重排属性/函数先后顺序，运行后需复查注释是否仍与对应代码位置对得上（如 property 上方的注释、函数前的注释）。

## 0. 全局禁令

- **不使用 Qt5Compat**：全局禁用 `QT += 5compat`（`QSanguosha.pro` 当前 `QT += network widgets quick quickwidgets`，不得添加 `5compat`，已在 .pro 注释标注）。所有需要的效果/组件用 Qt 6 原生方案（如 `MultiEffect` 走 `import QtQuick.Effects`），**不得引入 `Qt5Compat.GraphicalEffects` 等 compat 模块**。
- **QML 与 CPP 文件纯 ASCII**：所有 `.qml`/`.cpp`/`.h`/`.pro` 文件（含注释、字符串）必须使用纯 ASCII 字符，**不得包含中文、全角符号、em dash（`—`）、`§` 等非 ASCII 字符**。中文说明写在 `plan.md`（本文件）里，代码注释用英文。em dash 用 `--` 替代。
- **plan 只记最新状态**：除"进度记录"章节外，本文件其余部分（项目结构/任务清单/设计决策/代码流程等）记录当前实现/约定的最新状态，**不写代码取回/还原/重构的历史过程**，**不写已修正的 bug**（已修复的 bug 描述直接删除，不留"修复了 X"/"纠正误判"痕迹）。设计小节描述现状，不描述"曾经是 A 现在改成 B"。说明改动原因时用"旧版代码中 X 未做 Y。本次因 Z 加入"句式（陈述旧版事实+原因），不用"此前...→..."变迁叙述。"进度记录"章节不受此限，按时间顺序追加，保留历史动作描述（如"`.pro` OTHER_FILES 加 X"，即使后续改为 SOURCES 仍保留原条目）。进度记录只记"做了什么/确认了什么事实"，不记"之前错了现在改对"。进度记录不写编译/测试/运行情况（如"编译通过"/"lint 通过"）。

## 1. 项目概述
将旧版基于 QGraphics 的界面 `src/uibackup/`（35 对 .cpp/.h，约 17k 行，不编译，仅参考）重构为 Qt 6 QML。`MainWindow` 通过 `QQuickWidget` 加载 `qml/main.qml`，在 StartScene 与 RoomScene 间切换。核心布局（Photo/Dashboard/CardItem/StartScene/RoomScene）已成型，游戏交互（弹窗/聊天/技能按钮/卡牌选择）逐步移植中。

技术栈：Qt 6 QML（QtQuick 6.5），`import rocks.touhousatsu 1.0`；`QQuickWidget` + `qmlRegisterType`/`qmlRegisterSingletonType`/`qmlRegisterUncreatableType`；qmake（`QSanguosha.pro`）。

## 2. 项目结构

### C++ 侧
- `src/dialog/mainwindow.cpp`：构造 `QQuickWidget`，`setContextProperty` 暴露 `MainWindowInstance`/`Sanguosha`/`Config`，`setSource("qml/main.qml")`。提供 `qml_switchToRoomScene()` 信号。全局 `QPointer<MainWindow> MainWindowInstance`（仿 `RoomSceneInstance`，构造函数第一行赋值）。
- `src/qmlui/qmlui.cpp`：`Q_COREAPP_STARTUP_FUNCTION(registerCore)` 自动注册 QML 类型。
  - 单例：`G`（`TouhouKillQmlUiGlobal`，字体/游戏模式判断、`getAssetUrl(path)`/`assetExists(path)` 资源访问）、`ServerInfo`（含 `EnableAI`/`GameMode`/`FreeChoose` 等）。
  - 可创建：`CppRoomScene`（`RoomScene`，`QQuickItem` 桥接宿主）。
  - uncreatable：`Card`/`EquipCard`（暴露 `Location` 枚举）/`Player`/`ClientPlayer`/`General`/`Client`/`Skill`/`ViewAsSkill`/`FilterSkill`/`ProhibitSkill`/`DistanceSkill`/`MaxCardsSkill`/`TargetModSkill`/`AttackRangeSkill`。
- `src/qmlui/roomscene.h/cpp`：桥接层。`Q_PROPERTY` 暴露 `Self`（`ClientPlayer *`）、`ClientInstance`（`Client *`）；`connectClientSignals()` 集中连接 Client 信号；`Q_INVOKABLE replyToServer/notifyServer/freeChooseGeneral`。全局 `QPointer<RoomScene> RoomSceneInstance`。
- `src/client/client.h/cpp`：`Client : public QObject`，全局 `QPointer<Client> ClientInstance`（注释明确不应是单例，当前仍是）。约 50 个信号。`getPlayers()` 已 `Q_INVOKABLE`。`addPlayer`/`arrangeSeats`/`removePlayer` 玩家生命周期。
- `src/client/clientplayer.h`：`ClientPlayer : public Player`，全局 `QPointer<ClientPlayer> Self`。
- `src/core/player.h`：`Player` Q_PROPERTY 暴露 `seat`/`hp`/`renhp`/`linghp`/`maxhp`/`kingdom`/`role`/`general`/`general2`/`phase`/`alive`/`chained`/`avatar` 等。**`seat` 无 NOTIFY**；`avatar` MEMBER `m_avatar` NOTIFY `avatar_changed`；`phase`(QString, STORED false) 与 `phaseValue`(`Player::Phase` 枚举) 双属性。
- `src/core/protocol.h`：`QSanProtocol` namespace 用 `Q_NAMESPACE` + `Q_ENUM_NS(GameEventType)` 暴露游戏事件枚举给 QML（`qmlRegisterUncreatableMetaObject` 注册为 `QSanProtocol`，QML 用 `QSanProtocol.S_GAME_EVENT_ADD_SKILL` 等访问）；`CommandType` 枚举、`Countdown`。
- `src/core/util.h`：`IntList2VariantList` 等通用转换（桥接复用）。
- `src/uibackup/`：35 对死代码，不在 .pro（不编译），仅参考。

### QML 侧
- `qml/main.qml`：`Image` 背景 + `scalableRoot`（固定高 1440，宽随高缩放，最小 1920）+ `RootItem`。有"宽度过小提示"TODO。
- `qml/RootItem.qml`：`currentScene` 在 StartScene/RoomScene 间切换，监听 `MainWindowInstance.qml_switchToRoomScene`/`qml_switchToStartScene`。
- `qml/RoomScene.qml`：根 `CppRoomScene`。`property list<Photo> otherPhotos`（QTBUG-147713，配 `readonly property int zzzWorkaroundQTBUG147713: 0` workaround）。`lay()` 按 `effectiveSeat` 布局。`Component.onCompleted` 预创建占位 Photo（seat 2..N，未绑 player）。`Connections` 接收全部 `notify*`。`activeBox` 跟踪当前活动响应 box（按 status 自适应，同一时间只有一个）。`signal spaceClicked` + 根 MouseArea 空白点击触发（`RoleComboBox`/`HegRoleComboBox` 各自内部 `Connections { target: roomScene }` 监听以收起展开）。Dashboard 实例传 `photo: selfPhoto`。含 `testItemToBeRemovedAfterTest` 测试桩（`visible: !gameStarted`）。addRobot/fillRobots 已实现；未开始且未结算时显示"返回主菜单"按钮 `startSceneButton`（调 `MainWindowInstance.gotoStartScene()`）。
- `qml/Photo.qml`：`player` + `required property int seat` + `required property bool selfPhoto`（RoomScene 显式传 true/false：selfPhoto 实例 true，占位/测试 Photo false）。提取 `getGeneralName(g)`/`getImageSourceUrl(g)`（含 `_hegemony` 后缀处理）；source/visible 绑定；general2Image 对称 kingdom frame；player null fallback。`phase` 绑定 `player.phaseValue`（枚举）；未开始时 `general` 用 `player.avatar`；`PhaseItem` 用 `createPhaseItem()` 命令式创建（`Component.createObject`，`Component.onCompleted` 里 `if (!selfPhoto)` 调用——selfPhoto 不构造，用 Dashboard 的；无销毁；`visible` 绑 `gameStarted`；Component 模板内不访问外层 id 避开 `pragma ComponentBehavior: Bound` 告警）；`duozhi`（夺志，禁止角色使用/打出牌）时主副将图显示嘤嘤怪。化身图 `huashenImage`/`huashen2Image` source/visible 声明式绑定（`getImageSourceUrl(huashenGeneral)` / `huashenGeneral != ""`），`_hegemony` 由 `getImageSourceUrl` 统一 fallback；opacity 循环动画（500ms 淡入→4s 停→500ms 淡出→1s 间歇）由 `onVisibleChanged` 启停。
- `qml/Dashboard.qml`：4 按钮（OK/Cancel/Discard/Trust）为 **platter 按钮集合**（`bg.png` 100×195 背景 + 4 platter 按钮按 `skins/defaultSkin.layout.json` 的 `confirmButtonArea`/`cancelButtonArea`/`discardButtonArea`/`trustButtonArea` 叠放，对标旧版 `roomscene.cpp:800-815`）；`clientInstance` + `required property var photo`（绑 selfPhoto）+ `roomScene` 属性。`addHandCard`/`removeHandCard` 同步手牌（A 子任务）。`updateStatus()` 按 `Client::status` switch 分支命令式设 `okEnabled`/`cancelEnabled`/`discardEnabled`（`Connections` 监听 status/refusable/activeBox/canAccept）；`PhaseItem` 显示 `photo.phase`。
- `qml/CardItem.qml`：`signal clicked`/`rightClicked`（左/右键分发）；`selected` 属性注释掉（待实现）。
- `qml/EquipSlot.qml`：装备单槽显示（`property int cardId`，`onCardIdChanged` 经 `Sanguosha.getEngineCard` 取卡读 `objectName`/`suit`/`number_string`/`red`）。**可配置**（`equipIconDir`/`equipIconWidth`/`equipIconHeight`/`suitArea`/`pointArea`），由 `EquipAreaBase` Repeater 设配置（SelfEquipArea → dashboard `image/equips/` 149×25；PhotoEquipArea → photo `image/fullskin/small-equips/` 140×19）。对标旧 `_getEquipPixmap` + defaultSkin layout；**图标自带装备名**，无装备名 text、无距离 text（skin `equipTextArea`/`equipDistanceArea` = `[0,0,0,0]`）。不含距离/broken/点击分流（装备区任务）。
- `qml/EquipAreaBase.qml`：装备区基类（Item，5 槽 `Repeater` + `equipCardIds`/`addEquip(location,cardId)`/`removeEquip(location)` + 可配置 `equipIconDir`/`equipIconWidth`/`equipIconHeight`/`suitArea`/`pointArea`/`slotWidth`/`slotHeight`/`slotStep`；`height: 4*slotStep+slotHeight`/`width: slotWidth` 自适应）。动画预留：`Repeater.itemAt(location)` 取 EquipSlot 实例（card-fly 目标）。不含点击分流/有效态/broken/距离（装备区任务）。
- `qml/SelfEquipArea.qml`：self 装备区（`EquipAreaBase` 根，dashboard 默认配置 `image/equips/` 149×25）。进 Dashboard `equipBg` 左侧（`property alias equipArea` 暴露）。点击分流/有效态/broken/距离/装备技能按钮 TODO。
- `qml/PhotoEquipArea.qml`：非 self 装备区（`EquipAreaBase` 根，photo 配置 `image/fullskin/small-equips/` 140×19）。叠加 Photo 底部（`anchors.bottom`），`visible: !selfPhoto`，仅展示。
- `qml/GraphicsBox.qml`：图片背景可拖拽容器基类（Image 根，无标题/操作按钮）。
- `qml/ChooseGeneralBox.qml`：基于 GraphicsBox 的选将弹窗。
- `qml/QSanButton.qml`：按钮组件。`property url normalSource`/`hoverSource`/`downSource`/`disabledSource`（调用方指定各状态图，QSanButton 按 state 选取，空状态 fallback `normalSource`）+ `property bool overlayEnabled`（hover 叠加与 Text 显隐，platter 模式设 false）。**调用方直接指定各状态 image，不拼接路径、不约定文件名**：platter 按钮 4 个 source 都设 + `overlayEnabled: false`；普通按钮只设 `normalSource`（其他空 fallback），`overlayEnabled` 默认 true。
- `qml/TablePile.qml`：桌面牌堆（`PlaceTable`/`PlaceJudge`/`DiscardPile` 统一），Item 根内含 CardContainer（id `pile`）+ 延迟清除 Timer。`addCard`/`removeCard` 走 `pile.createItem`/`removeItem` + `lay(AlignHCenter)`；`clearTimestamps`（cardId->时间戳）+ `currentTime`（Timer 每秒+1）+ `_markOverflowClearance`（超量标记）+ `_checkClearance`（超时 `destroy` + 重排）。对标旧 `TablePile.cpp`。
- `qml/JudgeArea.qml`：判定区（延时锦囊），每张牌一个图标（不显示 CardItem），`cardIds` Repeater + `addDelayedTrick`/`removeDelayedTrick`（concat/filter 赋值触发更新）。图标 `image/icon/<objectName>.png`。
- `qml/PrivatePileArea.qml`：私人牌堆（`PlaceSpecial`），每个 pile 一个按钮（翻译名+数量）+ 点击弹下拉菜单显示 pile 牌（CardItem Repeater）。`player.getPileNames()`/`getPile(name)`（Q_INVOKABLE）+ `pile_changed` 驱动刷新。

### 构建
- `QSanguosha.pro`：`SOURCES`/`HEADERS` 含 `src/qmlui/*`；QML 文件在 `lupdate_only { SOURCES += ... }` 块（见下"lupdate QML 识别"）。`src/uibackup` 未引用。
- `compile_commands.json`：构建目录 `/Users/fs/build-QSanguosha-Qt_6-Release`，clang++ Qt6 arm64 macOS。
- **lupdate QML 识别**：`QSanguosha.pro` 用 `lupdate_only { SOURCES += <qml 文件列表> }` 块（非 `OTHER_FILES`，非 file glob）。lupdate-pro 扫描 SOURCES 的 qsTr，OTHER_FILES 不扫描；`lupdate_only` 块仅 lupdate 处理，qmake 构建忽略（.qml/.js 无编译规则，不会进入构建）。QML 翻译字符串用 `qsTr()`。

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
- [x] **国战双将势力校验**：`ChooseGeneralBox._canPair`（同势力或至少一方 `"zhu"`，对齐 `room.cpp:3723`/旧版 `choosegeneralbox.cpp:489`）+ `Engine::getGeneral` 加 `Q_INVOKABLE`（QML 直调 `Sanguosha.getGeneral(name).kingdom`，含 `_hegemony` fallback）；`_toggle` 拦截不合规第二将 + 不合规候选灰显。

### 待做（按优先级）
- [ ] **CardItem 卡牌选择（打通 OK/Cancel/Discard 全链路）**：参考旧版 `src/uibackup/roomscene.cpp` 的 `useSelectedCard()`(2479)/`doOkButton()`(3083)/`doCancelButton()`(3100)/`doDiscardButton()`(3193)/`updateStatus()`(2743) 与 `src/uibackup/dashboard.cpp` 的 `getSelected`/`pendingCard`/`startPending`/`unselectAll`/`enableCards`。整条链路依赖较多，拆为以下子任务（按依赖顺序）：

  - [ ] **A. 牌区同步**（前置，覆盖所有玩家可见牌区；手牌/装备/TablePile/判定区/私人牌堆 已做）：`Client::getCards`/`loseCards` 回调对应 `move_cards_got`/`move_cards_lost` 信号（桥接**尚未转发**），按 `CardsMoveStruct.to_place`/`from_place`（`Player::Place` 枚举：`PlaceHand`/`PlaceEquip`/`PlaceDelayedTrick`/`PlaceJudge`/`PlaceSpecial`/`PlaceTable`/`DiscardPile`）与 `from`/`to` player 分发到对应区域。不仅是手牌：
    - **手牌区**（`PlaceHand`）✅：Dashboard `cardArea`（CardContainer）增删 CardItem 并 `lay()`。其他玩家手牌对 self 不可见，只同步 `ClientPlayer.handcard` 数量（已有 `Q_PROPERTY` + `handcardChanged`，Photo 已绑）。**其他玩家手牌移动过程中 `cardId` 可能为 -1（故意设计，客户端数据不对等——隐藏其他玩家手牌信息）**，CardItem 需处理 `cardId=-1`（显示牌背 `image/system/card-back.png`，不显示卡面/花色/点数）；`qml/CardItem.qml:onCardIdChanged` 已处理 `cardId=-1` 显示牌背。self 自己的手牌 `cardId` 是真实值，正常显示卡面。
    - **装备区**（`PlaceEquip`）✅（同步部分）：Dashboard 5 槽（`qml/Dashboard.qml` 的 `equipCardIds[EquipCard.WeaponLocation]` 等 + `EquipSlot`，`image/equips/` 149×25）；其他玩家装备叠加 Photo 内部底部（`qml/Photo.qml` 非 self `equipArea`，`image/fullskin/small-equips/` 140×19）。槽位由 QML 读 `Sanguosha.getEngineCard(cardId).getRealCard().location` 获取（getEngineCard 返回 WrappedCard，须 getRealCard 拿真实 EquipCard 才有 location Q_PROPERTY）——桥接层 `move_cards_got`/`lost` 只转 move 字段，不解析装备槽位；QML `onNotifyMoveCardsGot/Lost` 按 `toPlayer`/`fromPlayer` 分发到目标 dashboard/photo 的 `addEquip`/`removeEquip`。装备槽对标旧 `_getEquipPixmap` + defaultSkin layout（dashboard/photo 两段），图标自带装备名，无装备名/距离 text（skin `equipTextArea`/`equipDistanceArea` = `[0,0,0,0]`）。**broken 占位图/距离文字/点击分流/有效态/装备技能按钮**留待"Dashboard 技能按钮 + 装备区"任务。
    - **判定区**（`PlaceDelayedTrick`）✅：放延时锦囊（闪电、乐不思蜀、兵粮寸断等）。每张牌一个图标（不显示 CardItem），对标旧 `PlayerCardContainer::addDelayedTricks`（图标 `image/icon/<objectName>.png`，`defaultSkin.image.json: judgeCardIcon-default`）。`qml/JudgeArea.qml` 容器（`addDelayedTrick`/`removeDelayedTrick` + `cardIds` Repeater，concat/filter 赋值触发更新）。self 在 Dashboard、其他在 Photo（`visible: !selfPhoto`）。RoomScene `onNotifyMoveCardsGot/Lost` 按 `toPlayer`/`fromPlayer` 分发到目标 `dashboard.judgeArea`/`photo.judgeArea`。
    - **TablePile（桌面牌堆，统一处理 `PlaceJudge`/`PlaceTable`/`DiscardPile`，不区分）**✅：判定牌（`PlaceJudge`，判定过程翻出的实体牌，分出此 Place 是为给"红颜"等修改实体判定牌的技能留位置）、桌面打出的牌（`PlaceTable`）、弃牌堆（`DiscardPile`）三者都由 TablePile 显示，不做区分。TablePile 直接用 CardContainer（见"CardItem 与牌容器设计"小节）。**弃牌不直接 destroy，走延迟清除 + 重排机制**（见"CardItem 与牌容器设计"小节的"TablePile 延迟清除机制"）；`PlaceTable`/`PlaceJudge`/`DiscardPile` 三者间互转（TablePile 内部 place 变化）跳过 add/remove，牌原位保留（不改顺序、不重启动画）。进出场动画（新建+销毁，无源位置飞行）待"CardItem 移动动画"任务统一处理。
    - **私人牌堆**（`PlaceSpecial`，如邓艾"屯田"的田、木牛流马 `wooden_ox`）✅：`Player::getPileNames()`/`getPile(name)` 加 `Q_INVOKABLE`（QML 直调）；`pile_changed(name)` 信号（`changePile`/`setPile` 触发）QML `Connections` 监听刷新。`qml/PrivatePileArea.qml`：每个 pile 一个按钮（翻译名+数量），点击弹**下拉菜单**显示 pile 牌（CardItem Repeater，cardId=-1 显示牌背）；pile 牌变化由 `pile_changed` 驱动（不走 move 分发）。**每个 Photo 一个 PrivatePileArea 实例**（self/非self 都在各自 Photo 旁显示，Dashboard 不另建）。
    - **公共牌区 / 五谷丰登（AG）**：由 `notifyAgFilled`/`notifyAgTaken`/`notifyAgCleared` 驱动（**已桥接**，见 `roomscene.h`），**不走 `move_cards_got/lost`**，AG 区不属于任何 player 的 place。
      - **旧代码 `PlaceWuGu` 情况**：旧版 `src/uibackup/roomscene.cpp:3902` 的 `takeAmazingGrace()` 用 `Player::PlaceWuGu` 作 `CardsMoveStruct.from_place` 标记牌来源（五谷丰登公共堆），**仅用于客户端取牌动画**；**`src/core/player.h` 的 `Place` 枚举不含 `PlaceWuGu`**（旧 uibackup 死代码引用了已不存在的枚举值，服务器侧也从不发送 PlaceWuGu，五谷丰登走 AG 机制 `S_COMMAND_FILL_AMAZING_GRACE`/`S_COMMAND_TAKE_AMAZING_GRACE`，对应 `ag_filled`/`ag_taken` 信号）。
      - **本次重构不引入 `PlaceWuGu` 即可保持功能**：`notifyAgTaken(taker, cardId, moveCards)` 已携带取牌动画所需的全部信息——`moveCards=true` 时 taker 手牌区增牌（走手牌同步，`to_place=PlaceHand`），取牌动画起点用 AG box 自身牌位置（QML 侧已知），无需专门的 `from_place` 枚举值；`moveCards=false` 时仅 AG 区移除该牌。AG box 是独立组件（待做，归入"玩家牌展示/桌面牌堆"任务的 PlayerCardBox/GenericCardContainer 范畴），A 子任务只需保证 `notifyAgTaken` 能驱动 taker 手牌区增删即可。
    - 实现要点：补桥接 `notifyMoveCardsGot`/`notifyMoveCardsLost`（转发 `move_cards_got`/`move_cards_lost`）。**`CardsMoveStruct` 在 C++ 桥接层处理，不暴露给 QML**（不 `qmlRegisterUncreatableType`）——桥接层解析每个 move，转成 QML 可理解的字段传给 QML：`cardIds` (QVariantList&lt;int&gt;)、`fromPlace`/`toPlace` (int，`Player::Place` 枚举值，`Q_ENUM` 已可用，QML 用 `Player.PlaceHand` 等比较)、`fromPlayer`/`toPlayer` (`Player *`，桥接层用结构体已有的 `from_player_name`/`to_player_name` 调 `ClientInstance.getPlayer(name)` 查得；`Player` 已 `qmlRegisterUncreatableType` 注册，QML 可直接读其 Q_PROPERTY)、`fromPileName`/`toPileName` (QString) 等。QML 侧 `onNotifyMoveCardsGot/Lost` 遍历 moves，按 `toPlace`/`fromPlace` + player（self vs 其他 Photo）分发到对应区域，增删后 `lay()`。
    - **`CardsMoveStruct` 的 `from`/`to` 指针**：`tryParse`(`src/core/structs.cpp:9`) 只解析 `from_player_name`/`to_player_name` 字符串，不解析 `from`/`to` 指针；但 `Client::getCards`/`loseCards`(`src/client/client.cpp:553-554`) 在 tryParse 后补设 `move.from = getPlayer(from_player_name)` / `move.to = getPlayer(to_player_name)`，所以 `move_cards_got`/`move_cards_lost` 信号触发时 moves 里的 `from`/`to` **非 null**（是 `ClientPlayer *`，也是 `Player *`）。桥接层直接用 `move.from`/`move.to` 作 `fromPlayer`/`toPlayer` 传给 QML（`Player` 已注册，QML 可读其 Q_PROPERTY），无需再 getPlayer。结构体 `from`/`to` 的 `Player *` 虚基类 dynamic_cast 问题（`src/core/structs.h:328`）后续细查。
    - **`isLastHandcard` 不传**：`tryParse` 未解析该字段（客户端恒为构造默认 false），客户端无使用，桥接层不传给 QML。
    - **不用 `CardsMoveStruct::toVariant()`**：该函数（`src/core/structs.cpp:37`）是服务器序列化用，输出 `JsonArray`（服务器协议格式，非 QML 语义化字段），且 `structs.cpp` `#include "room.h"` 可能有依赖。查证 `toVariant` 本身不直接用 `ServerPlayer`（只读 `card_ids`/`shown_ids`/`from_place`/`to_place`/`from_player_name`/`to_player_name`/`from_pile_name`/`to_pile_name`/`reason`/`open`），但格式不适合直接给 QML。桥接层自己解析这些字段转 QML 可理解内容。
    - **CardItem 与牌容器设计（前置）**：见"设计决策与约定"的"CardItem 与牌容器设计"小节——CardContainer 作为通用牌容器（TablePile 直接用，AG box 等 = GraphicsBox 内嵌 CardContainer）；CardItem 的 QObject parent 统一在 roomScene，移动用原实例（visual parent 切换：container → roomScene 飞行 → 新 container），不显示的牌直接 destroy。**`qml/CardContainer.qml:10` 的 `createItem` 现有 `createObject(this)` 必须改为 `createObject(roomScene)` + `cardItem.parent = this`**（否则牌移走后 QObject parent 还在 CardContainer，涉及转移问题）。
    - 无牌区同步则无从选卡/选装备，是 B-E 的前置。
  - [ ] **B. CardItem 选中态**：启用 `qml/CardItem.qml` 注释掉的 `selected` 属性 + `selectedChanged` 信号；单击 toggle `selected`；`enabled` 已联动禁用遮罩（见 2026-07-19 进度）。选中视觉对标旧版 `Dashboard::selectCard`(`uibackup/dashboard.cpp:538`)：
    - **抬起**：对标 `S_PENDING_OFFSET_Y = -25`(`dashboard.h:128`)。用 `transform: Translate { y: selected ? -25 : 0 }` + `Behavior` 动画，**不动 `y`/`homeY`**，避免与 `lay()`/`goBack()` 冲突（旧版改 homePos 会与 lay 重设打架，QML 用 transform 隔离更干净）。手牌区用。
    - **不使用 glow**：旧版 `QGraphicsDropShadowEffect`(`carditem.cpp:373`，Qt 自带 `QGraphicsEffect` 子类) 是 **hover 触发**（`hoverChanged → setEnabled`），非 selected 触发，故不引入 glow（不引入 `MultiEffect`/`QtQuick.Effects`；不接受 `Qt5Compat`）。
    - **ChooseGeneralBox 改造**：去掉 `qml/ChooseGeneralBox.qml:99` 的 `scale: ... ? 1.1 : 1.0`，改为 `selected: selectedGenerals.indexOf(modelData) >= 0`。ChooseGeneralBox 的 GridView `clip: true`，抬起 25 像素会被裁剪，故改用 **border 高亮**（selected 时 CardItem 加金色/白色 border）替换 scale 1.1，与手牌区抬起不同的 selected 视觉（按上下文）。
    - `qml/CardContainer.qml` 暴露 `selectedItems`（只读）/ `unselectAll(except)` / `selectOnlyCard()`。
  - [ ] **C. Dashboard pending 状态机 + PromptBox**：扩展 `Dashboard.updateStatus()` 按 status 进入 pending（镜像旧 `updateStatus` 各 case）：`Playing` → `enableCards`；`Responding`/`Discarding`/`Exchanging`/`AskForShowOrPindian` → `startPending(skill)` 按当前 pattern。手牌按 ViewAsSkill `viewFilter` enable/disable。pattern 来源 `Sanguosha->currentRoomState()->getCurrentCardUsePattern()`，需在 C++ 侧（`RoomState` 或桥接）暴露 `Q_PROPERTY`/`Q_INVOKABLE` 给 QML。新建 `qml/PromptBox.qml` 显示 prompt 文本（`Client::getPromptDoc()` 是 `QTextDocument*`，需桥接转 QString，参考旧 `roomscene.cpp:320`），随 status `appear`/`disappear`。
  - [ ] **D. 桥接选卡回传**：`const Card *` 无法跨 QML，桥接层 `RoomScene` 新增 `Q_INVOKABLE` 回传接口（如 `submitCardResponse(int status, QVariantList cardIds, QStringList targetNames)` / `submitDiscard(QVariantList cardIds)` / `cancelResponse(int status)`）。内部按 `cardIds` 经 `Client::getCard` 取 `const Card *`，`targetNames` 经 `Client::getPlayer` 转 `const Player *`，调 `Client::onPlayerResponseCard`/`onPlayerDiscardCards`/`onPlayerInvokeSkill` 等。**第一阶段不做 ViewAsSkill 组合牌**（如"龙鳞"用两牌当杀），只回传单张原始牌；组牌留待 G 子任务。
  - [ ] **E. OK/Cancel/Discard 按钮回传**：Dashboard 三按钮 `onClicked` 按 `clientInstance.status` 分支调桥接：OK → `submitCardResponse`/`submitDiscard`/`onPlayerInvokeSkill(true)`（AskForSkillInvoke）/`activeBox.accept()`（AskForGeneralTaken）；Cancel → `cancelResponse(status)`（`Playing`=unselectAll 不回传、`Responding`/`AskForShowOrPindian`=`onPlayerResponseCard(nullptr)`、`Discarding`/`Exchanging`=`onPlayerDiscardCards(nullptr)`、`ExecDialog`=reject dialog、`AskForSkillInvoke`=`onPlayerInvokeSkill(false)`、`AskForPlayerChoose`=`onPlayerChoosePlayer(nullptr)`）；Discard → `Playing` 状态 `onPlayerResponseCard(nullptr)`（结束出牌阶段）。
  - [ ] **F. 目标选择**：Photo 单击进入目标选择（status=`Playing`/`Responding` 且当前选中卡需目标，按 `card.targetFixed(Self)` 判断）；收集 `selected_targets`；Photo 视觉高亮可选目标（按 `card.targetFilter`，桥接 `Q_INVOKABLE enabledTargets(cardId)` 返回可选 player objectName 列表）；选满后 OK 启用。
  - [ ] **G. 技能按钮触发的 ViewAsSkill（与"Dashboard 技能按钮"任务交叉，合并到该任务）**：`QSanSkillButton` 点击 → `startPending(skill)`，组牌回传。依赖技能按钮组件。

  **简化策略（第一阶段最小闭环）**：A → B → C → D → E（先只支持 `targetFixed` 的卡：桃自用、无懈可击等），再 F 补目标选择（杀/闪/桃救人等），G 与"Dashboard 技能按钮"任务合并。
- [ ] **CardItem 移动动画**（牌区同步的动画完善，A 子任务后续）：当前手牌/装备/TablePile 的进出场是简化"新建+销毁"模式——牌进区域时 `createItem` 在容器原点(0,0)创建后 `lay()`+`goBack()` 飞向 `homePos`（起点不自然，非从源位置飞来）；牌离区域时直接 `removeItem` destroy（无飞出动画）。本任务统一改为"原实例移动"（见"CardItem 与牌容器设计"小节）：容器间移动用原 CardItem 实例，切 visual parent 前后用 `mapToItem`/`mapFromItem` 把牌当前位置映射到目标容器坐标系作动画起点，`lay()` 设 `homeX`/`homeY`，`goBack()` 飞行；无目标的牌 `destroy()`。涉及手牌区（`Dashboard.addHandCard`/`removeHandCard`）、装备区（`addEquip`/`removeEquip`）、TablePile（`addCard`/`removeCard`）的 add/remove 改造，以及 `move_cards_got`/`lost` 配对 move 的源→目标实例传递（当前 got 新建 + lost 销毁，需改为 got 接收 lost 释放的实例）。TablePile 延迟清除已改为直接 `destroy()`（不淡出），本任务不重做。**待排查**：TablePile 仍有全透明 carditem 残留（`_checkClearance` 跨对象 list 副本 splice 已修但仍存在），原因待技能实现后排查（可能与技能发动的 move 流程或 CardItem opacity 动画有关）。
- [ ] **选项/触发顺序弹窗**：ChooseOptionsBox（askForChoice/askForOrder/askForDirection/askForSuit/askForKingdom）、ChooseTriggerOrderBox（askForTriggerOrder）。
- [ ] **玩家牌展示/桌面牌堆**：PlayerCardBox（showAllCards/showCard/askForGongxin）、GenericCardContainer、TablePile（askForGuanxing/askForYiji）。
- [ ] **聊天与日志**：ChatWidget、BubbleChatBox、ClientLogBox。
- [ ] **Dashboard 技能按钮 + 装备区**（含 CardItem 全链路 G 子任务）：旧版实现参考见"装备区与技能按钮（旧版实现参考）"小节。子任务：
  - [x] **装备区拆两套实现**（组件结构部分）：`qml/EquipAreaBase.qml`（基类，5 槽 Repeater + `equipCardIds`/`addEquip`/`removeEquip` + 可配置图标/布局，动画预留 `Repeater.itemAt`）+ `qml/SelfEquipArea.qml`（EquipAreaBase 根，dashboard 配置，进 Dashboard `equipBg`，`property alias equipArea` 暴露）+ `qml/PhotoEquipArea.qml`（EquipAreaBase 根，small-equips 配置，叠加 Photo 底部 `visible: !selfPhoto`，仅展示）。Dashboard/Photo 不再持 `equipCardIds`/`addEquip`/`removeEquip`（移入 EquipAreaBase）；RoomScene 调 `dashboard.equipArea`/`photo.equipArea` 的 `addEquip`/`removeEquip`。**未做**：点击分流/有效态/broken 占位图/距离文字/装备技能按钮/Photo 动态 createObject（依赖 G 子任务）。
  - [ ] **broken_equips 暴露**：`player.h` `broken_equips`/`isBrokenEquip` 加 `Q_PROPERTY`/NOTIFY（当前仅 `brokenEquips_changed` 信号）；桥接转发（self/非self 都需，Photo 装备也要 broken 占位图）。通知链：服务器 `S_COMMAND_SET_BROKEN_EQUIP` → `Client::setBrokenEquips`(`client.cpp:251`) → `Player::setBrokenEquips` → `brokenEquips_changed`（**走 Player 信号，非 Client notify 信号**）。
  - [ ] **装备有效/无效态**：(a) ViewAsSkill pending 时按 `viewFilter` 判装备 markable，不可 mark 且装备技能按钮不可用 → 灰显（旧 opacity 0.7）；(b) broken 占位图。非 self 无 pending 态（仅 broken 占位图）。
  - [ ] **装备点击分流（双重功能）**：对标旧 `Dashboard::mouseReleaseEvent`(`uibackup/dashboard.cpp:770-794`)——装备技能按钮存在且 enabled → 发动装备技能；否则 markable → mark 选中（ViewAsSkill 素材/弃装备代价）。**同一点击入口按状态分流**（非两个独立可点击层）。装备技能按钮旧版不挂场景（`new QSanInvokeSkillButton()` 无 parent），不显示自身，视觉反馈靠装备边框动画 `image/system/emotion/equipborder/`。
  - [ ] **`qml/QSanSkillButton.qml`**（多状态，装备/武将共用同类）：(1) 装备技能按钮（绑 self 装备槽，skill=`Sanguosha.getSkill(equip)`，`equip_skill` 标志，旧版不挂场景不显示自身，视觉反馈靠装备边框动画）；(2) 武将技能按钮进**单 dock**（锚定 selfPhoto、底部对齐，parent = RoomScene）。**不做主将/副将双 dock**——QML 版 Dashboard 已去除武将图部分改用 selfPhoto，旧版双 dock（主将 dock 在 self 头像下方 + 副将 dock 在副头像下方，`dashboard.cpp:265-279`）会违和。SkillType 6 种（PROACTIVE/FREQUENT/COMPULSORY/AWAKEN/ONEOFF_SPELL/ARRAY/ATTACHEDLORD）× ButtonState 5 种（UP/HOVER/DOWN/CANPRESHOW/DISABLED），`setSkill` 按 skill 类型设 SkillType+Style(PUSH/TOGGLE)+初始 State+emitActivate/Deactivate（`qsanbutton.cpp:263-348`）。技能按钮视觉完整（旧 `_repaint`/`paint` `qsanbutton.cpp:377-477`：`getSkillButtonPixmap` 皮肤图+技能名+ATTACHEDLORD 武将头像+失效红叉+CANPRESHOW 黄框），武将技能按钮显示皮肤图+技能名，装备技能按钮不显示自身。非装备技能进单 dock。
  - [ ] **TODO（后续追加）**：attachlord 单独 dock、武将图显示等旧版有但 QML 暂不做的功能，后续追加。
  - [ ] **桥接 add/remove**：转发 `skill_attached`→add、`skill_detached`→remove、`skill_acquired`(self)→add、`skill_invalidity_changed`(self)→update；暴露 Self 可见技能列表 + 每个 Skill 的 viewAsSkill/dialog/SkillType；`UPDATE_SKILL`/`PREPARE_SKILL` 全量重建（过滤 LordSkill、国战 preshow、非国战全 disable）。
  - [ ] **ViewAsSkill pending + 互斥 + chooseSkillButton**：按钮点击 → viewAsSkill 非空 → `startPending`（装备按 viewFilter 可 mark 作素材，对标旧 `startPending`/`updatePending`/`onMarkChanged`）；互斥（激活任一按钮 → dock 其他 ViewAsSkill 重置 + 其他装备技能按钮禁用，对标 `skillButtonActivated`/`Deactivated`）；`AskForSkillInvoke` 多按钮 → chooseSkillButton 弹窗。组牌回传扩展 D 子任务。
- [ ] **选将扩展**：askForGeneral3v3、askForRole3v3；KnownBoth（知己知彼卡牌效果，非国战双将）。
- [ ] **RoomScene 收尾**：移除 `testItemToBeRemovedAfterTest`、main.qml 宽度过小提示。
- [ ] **清理**：`src/uibackup` 死代码整体删除。
- [ ] **移 handleGameEvent 其余 UI 无关事件到 Client**：`S_GAME_EVENT_UPDATE_PRESHOW`（`Self->setSkillPreshowed`）、`S_GAME_EVENT_CHANGE_GENDER`（`player->setGender`）、`S_GAME_EVENT_CHANGE_HERO`（player 状态）等 UI 无关逻辑从旧 `RoomScene::handleGameEvent` 移到 `Client::handleGameEvent`（技能 4 事件已移）。
- [ ] **Client 去单例化**（QML 重构完成后着手，独立阶段）：`client.h` 末尾 TODO 注释明确"Client should ABSOLUTELY NOT be a singleton"——当前 `extern QPointer<Client> ClientInstance` 全局指针导致无法实现客户端侧 AI agent（只能服务端 AI）。改造方向：通过参数/上下文传入 Client 引用，移除全局 `ClientInstance`。**桥接层已部分铺垫**：`RoomScene::selfHelper()`/`clientHelper()`（注释"needed to refactor Self and ClientInstance from singleton"）当前返回 `Self`/`ClientInstance`，是去单例化的注入点；`clientHelper` 内亦有 TODO"consider how to get this after Client is no longer global singleton"。**注意自包含 dialog**：`RoleAssignDialog` 等 dialog 内部硬编码 `ClientInstance`（见"桥接架构"小节），需一并改造。

## 4. 设计决策与约定（讨论沉淀，后续必须沿用）

### 桥接架构
- **桥接集中在 CppRoomScene**：Client 信号统一在 `connectClientSignals()` 连接，`emit notify*` 转发 QML；QML 不直接依赖 Client 单例。符合"去单例化"方向（Client.h 注释明确不应是单例）。
- **notify 信号用具体类型**：参数用 `ClientPlayer *` 而非 `QObject *`（QML 类型提示好）；`Q_PROPERTY` 的 `Self`/`ClientInstance` 同理。
- **player 维护在 Client**：QML 不 accumulate 玩家副本，通过 `ClientInstance.getPlayers()`（已 Q_INVOKABLE）查询消费。
- **回传方式**：通用回传用 `replyToServer(int commandType, QVariant)`；简单请求直接调 Client slot（如 `addRobot()`/`trust()`/`onPlayerChooseGeneral(name)`）。
- **自包含 dialog（Client 去单例化时需处理）**：`RoleAssignDialog` 内部 accept() 直接调 `ClientInstance->onPlayerAssignRole(names, roles)`、reject() 调 `replyToServer(S_COMMAND_CHOOSE_ROLE, QVariant())` 自行回传服务器，因此桥接 `showRoleAssignDialog()` 返回 void、QML 无需返回值或后续处理。这与 `FreeChooseDialog`（桥接捕获 `general_chosen` 信号返回给 QML）模式不同。**Client 去单例化时**：此类 dialog 内部硬编码 `ClientInstance` 全局指针，必须改造为通过参数/上下文传入 Client 引用，否则会破坏；桥接层届时可考虑统一收集这类 dialog 的回传路径。**新增 dialog 一律走桥接收集回传模式（如 `FreeChooseDialog`），不内部硬编码 `ClientInstance`**，避免去单例化时技术债累积。
- **Engine 函数暴露给 QML**：遇到 Engine（`Sanguosha` 全局单例，已暴露给 QML）中 QML 需要调用但不可调用的函数，**直接在 Engine 类上加 `Q_INVOKABLE` 或 `Q_SLOT`，不走 RoomScene 桥接层**。判定：纯查询/无副作用/不需信号连接的加 `Q_INVOKABLE`（如 `getGeneral`）；需要被信号连接或有槽语义的加 `Q_SLOT`。Engine 已是 QML 可直接访问的全局对象，加宏后 QML 即可调用；返回的 QObject 子类需 `qmlRegisterUncreatableType` 注册后 QML/qmllint 才能识别其类型并读 `Q_PROPERTY`（`General` 已注册，可读 `kingdom`/`maxhp`/`gender`/`lord` 等）。

- **游戏事件 UI 无关逻辑在 Client**：旧版 `RoomScene::handleGameEvent`(`uibackup/roomscene.cpp:421`) 混了 UI 无关（`player->addSkill`/`loseSkill`/`acquireSkill`/`detachSkill`/`setSkillPreshowed`/`setGender` 等 player 状态更新）与 UI 相关（`updateAvatarTooltip`/`expandSpecialCard`/`updateSkillButtons` 等）。QML 重构把 UI 无关部分移到 `Client::handleGameEvent`(`client.cpp:330`)，`emit event_received` 后由 QML 桥接转发 notify 信号驱动 UI。技能 4 事件（ADD/LOSE/ACQUIRE/DETACH SKILL）已移；其余 UI 无关事件（preshow/gender/hero 等）TODO。
- **notify 处理函数的 log 约定**：`RoomScene.qml` 的 `onNotifyXxx` 占位时只放 `console.log` 作待实现标记；一旦为该 notify 添加了实际动作（调桥接/创建组件/遍历 Photo 等），**顺手删除其 `console.log`**（动作本身已表明信号到达，log 冗余）。
- **枚举类型暴露给 QML**：C++ 侧的枚举（如 `EquipCard::Location`）需 `Q_ENUM`（非旧式 `Q_ENUMS`）+ `qmlRegisterUncreatableType<T>` 注册后，QML 才能用 `T.EnumMember` 访问（如 `EquipCard.WeaponLocation`）。**`Q_ENUM` 必须置于 `enum` 声明之后**（旧式 `Q_ENUMS` 位置无关，但 `Q_ENUM` 在 enum 前编译不过）；`EquipCard` 是抽象类（`virtual location()=0`），注册为 uncreatable 仅暴露枚举，不创建实例。UI 侧装备槽索引用 `equipCardIds[EquipCard.WeaponLocation]` 等枚举名，不用魔数 0-4。**枚举值 Q_PROPERTY**：`EquipCard` 加 `Q_PROPERTY(Location location READ location STORED false)`，QML 读 `card.location`（`const Card *` 实际为 EquipCard 时，运行时元对象查找子类 Q_PROPERTY）直接得槽位，桥接层无需 dynamic_cast 解析。`Weapon` 加 `Q_PROPERTY(int range READ getRange)` 供距离显示（A 子任务暂不用，留装备区任务）。

### Qt6 moc / QML 约束
- **Q_PROPERTY 指针类型需完整定义**：`Q_PROPERTY(T *)` 中 `T` 必须 include 完整定义，不能前置声明（否则 moc `static_assert(is_complete<...>)` 失败）。`roomscene.h` 需 `#include "client.h"`/`"clientplayer.h"`。
- **`Player.seat` 无 NOTIFY**：seat 变化 QML 无法自动感知，必须在 `notifySeatsArranged` 显式读 `photo.player.seat` 回填 `photo.seat`。
- **NOTIFY 信号规范**：无参或单参（新值）；仅值变化时 emit。`status_changed(Status newStatus)` 单参，`setStatus` 保留 `old_status` 仅 `old != new` 时 emit。
- **避免 const_cast**：信号参数需非 const 传 QML 时，直接改 Client 信号签名去 const（如 `cards_got`/`skill_acquired`）。
- **`QList<int>` 转换**：复用 `util.h::IntList2VariantList`，勿手写循环。
- **读 Card 子类属性必须 getRealCard**：`Sanguosha.getEngineCard(id)`/`getCard(id)` 返回 **WrappedCard**（游戏中所有实际卡都是 WrappedCard，card.h 注释），不是真实卡子类（EquipCard/Weapon 等）。WrappedCard 透传 Card 基类属性（suit/number/objectName/number_string/red 等，takeOver 时同步），读基类属性可直接 `card.suit` 等；但**子类独有属性**（`EquipCard.location`、`Weapon.range` 等）WrappedCard 没有，`card.location` 返回 undefined。必须 `card.getRealCard()`（`Q_INVOKABLE`，WrappedCard override 返回 `m_card` 真实卡）拿真实卡后再读子类属性：`card.getRealCard().location`。
- **QML `property list<T>` 跨对象访问返回副本**：`var items = obj.listProp; items.splice/push(...)` 作用于副本，不修改原 list property。修改 list property 必须同对象直接操作（obj 内部用 `listProp.splice`、或经 obj 提供的方法如 `CardContainer.removeItem`），不可用跨对象副本 splice。
- **Player pile 接口暴露给 QML**：`Player::getPile(name)`/`getPileNames()` 加 `Q_INVOKABLE`（QML 直调，返回 QVariantList/QStringList）；`pile_changed` 信号（ClientPlayer）QML `Connections` 监听。PrivatePileArea 用此驱动 pile 按钮刷新，不走桥接 move 分发（`Client::getCards`/`loseCards` 调 `changePile` 已 emit `pile_changed`）。

### 选将流程约定
- **不用 ExecDialog**：`askForGeneral` 统一 `setStatus(AskForGeneralTaken)`。
- **OK 按钮复用 Dashboard 的**：响应 box（ChooseGeneralBox 等）不自带 OK，通过 `roomScene.activeBox`（通用，按 status 自适应，同一时间只有一个）跟踪，Dashboard OK 按钮触发 `activeBox.accept()`。
- **single_result 语义**：非国战/平异 = `true`（单将）；国战双将 = 服务器给定。`askForGeneral` 非国战分支需设 `single_result = true`。
- **国战双将势力校验**：双将必须同势力（kingdom），或至少一方为 `"zhu"`（百搭势力）。规则与服务器 `room.cpp:3723`、旧版 `uibackup/choosegeneralbox.cpp:489` 一致。QML 侧 `ChooseGeneralBox._canPair(g1, g2)` 直接调 `Sanguosha.getGeneral(name).kingdom`（`Engine::getGeneral` 已加 `Q_INVOKABLE`，QML 可直查，不判空不过度防御）；`_toggle` 选第二个将时拦截不合规搭配。**灰显**：`_isDimmed(g)` 联动 CardItem 标准 `enabled` 属性（`enabled: !_isDimmed(g)`），禁用时半透明黑遮罩覆盖（`visible: !enabled`，不用 opacity 避免漏 GraphicsBox 背景）——0 选不禁、1 选禁不可搭配、2 选禁全部未选。用 `enabled` 而非自定义属性，因 CardItem 作为手牌/装备等其他牌时也需禁用机制，统一复用。**OK 启用**：`canAccept`（单将≥1、双将=2）绑定 Dashboard OK 按钮 enabled 与 accept 校验，选未满不可确认。
- **回传格式**：单将 `name`，双将 `name1+name2`（与旧版 `reply()` 一致）。
- **右键 freechoose**：CardItem `rightClicked` 信号 + `ServerInfo.FreeChoose` → 调 `parent.freeChooseGeneral()`（C++ FreeChooseDialog modal exec）换将该位。
- **KnownBoth 是"知己知彼"卡牌效果**（非国战双将）；国战双将选择是独立需求。
- **国战将名 `_hegemony` 后缀是合法将名，`getGeneral` 查询不可去尾**：`Sanguosha.getGeneral("xxx_hegemony")` 直接查到国战将该本身；`xxx` 与 `xxx_hegemony` 是不同武将，kingdom 可能不同，去尾会查到错误的 general。注意这与 `Photo.qml` 的图片资源/翻译层面的 `_hegemony` 处理不同（图片资源可去尾 fallback 找文件、翻译可去尾查找 key，二者均非 getGeneral 查询）。

### UI 约定
- **GraphicsBox 基类**：Image 根（直接用 source 属性）、可拖拽、无标题/操作按钮/信号、default property content 槽位、Component.onCompleted 居中（x/y 而非 anchors，兼容拖拽）。
- **响应 box 接口约定**：所有需 Dashboard OK 确认的响应 box（ChooseGeneralBox/ChooseOptionsBox/ChooseTriggerOrderBox 等，多基于 GraphicsBox）须提供统一接口：`property bool canAccept`（当前选择是否可确认）+ `function accept()`（确认并回传/清理）。创建时 `roomScene.activeBox = box`，accept/销毁时 `roomScene.activeBox = null`。Dashboard 只认 `activeBox.canAccept`/`activeBox.accept()`，新增 box 类型不需改 Dashboard。同一时间只有一个活动 box（按 status 自适应）。
- **Dashboard 按钮**：`anchors.bottom: cardArea.top` + `bottomMargin` + `horizontalCenter` 浮在手牌区上方；268×133，font.pixelSize 50。**enabled 按 `Client::status`**：Discard=`Playing`；Cancel=`ExecDialog`/`AskForSkillInvoke` 或（`Responding`系列/`Discarding`/`Exchanging` 且 `discardActionRefusable`）；OK=活动 box `canAccept` 或 `AskForSkillInvoke`（其他响应状态待 CardItem 选卡落地后按选卡启用）。`Client::Status` 经 `Q_ENUM` + uncreatable 注册，QML 用 `Client.Playing` 等枚举名比较；`discardActionRefusable` 经 `Q_PROPERTY`（READ `isDiscardActionRefusable`/setter/NOTIFY `discardActionRefusableChanged`）暴露。enabled 由 `Dashboard.updateStatus()` 命令式设置（`okEnabled`/`cancelEnabled`/`discardEnabled` property），`updateStatus()`（switch case，含未来 prompt/card-pending/skill/target 复杂逻辑）只由 `status_changed` 调用；`onCanAcceptChanged` 直接设 `okEnabled = activeBox.canAccept`；不监听 `onDiscardActionRefusableChanged`（cancel 由 updateStatus 在 status 变化时重算）；`Component.onCompleted`/`onClientInstanceChanged` 直接置三按钮 false（等 `status_changed` 触发 updateStatus）。activeBox 创建/销毁不触发。。`updateStatus` 是 status 变化统一入口（镜像旧 `RoomScene::updateStatus`），后续 prompt/card pending/skill/target 选择等逻辑在此补；OK 点击在响应状态走目标选择等复杂流程，不只靠绑定。
- **资源访问**：统一用 `G.getAssetUrl(path)`。
- **`z` 属性使用约定**：**静态创建的对象**（QML 文件直接声明的元素）不使用 `z`，靠声明顺序/父子层级（后声明的同级元素渲染在上层）解决覆盖。**动态创建的对象**（`createObject`/Loader 运行时创建）可以使用 `z`，但严格限制使用范围——仅用于声明顺序无法解决的动态堆叠场景（如动态卡牌堆叠顺序 `CardContainer.lay` 的 `z = i`、动态弹窗置顶），不滥用。**历史原因**：旧代码（`src/uibackup/`）滥用 `z` 调整堆叠，已用到小数点前 5 位（万级），故对静态对象禁用以避免重蹈；动态对象放宽因运行时创建顺序无法在文件中靠声明顺序控制。
- **selfPhoto vs 非 selfPhoto 构造模式**：selfPhoto 与非 selfPhoto 共用 `Photo.qml` 组件，通过 `required property bool selfPhoto` 区分（RoomScene 显式传：selfPhoto 实例 `true`，占位/测试 Photo `false`）。**selfPhoto 不构造的子组件，由 Dashboard 承担显示**；非 selfPhoto 用 `Component.createObject` 命令式**动态构造**（`createPhaseItem()`/`createEquipArea()`/`createJudgeArea()` 在 `Component.onCompleted` 调用一次创建，无销毁；`visible` 绑条件控制显隐；Component 模板内不访问外层 id，避开 `Loader` 的 `pragma ComponentBehavior: Bound` 告警）。**selfPhoto 的对应 property 绑 Dashboard 的实例**（RoomScene selfPhoto 声明 `equipArea: dashboard.equipArea`/`judgeArea: dashboard.judgeArea`/`phaseItem: dashboard.phaseItem`，Photo 用 `property var` 非 alias 接收；Dashboard 用 `property alias` 暴露子项 id）。已应用：`PhaseItem`（selfPhoto 绑 `dashboard.phaseItem`；非 selfPhoto `createPhaseItem`）、`EquipArea`（selfPhoto 绑 `dashboard.equipArea`；非 selfPhoto `createEquipArea` 构造 `PhotoEquipArea`）、`JudgeArea`（同 EquipArea 模式）。
- **layout 仅参考旧代码，不严格对标**：旧 `defaultSkin.layout.json`/`SkinBank` 的坐标/尺寸仅作参考，QML 侧不逐字段对标。现有 QML 内容（Photo/Dashboard/EquipSlot/装备区等）layout 多有偏差（尺寸/间距/位置未精确还原旧版），功能做完后统一调整，不在各子任务中纠结 layout 精确度。

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
  - **TablePile 延迟清除机制（牌进弃牌堆不直接 destroy）**：牌进 TablePile（`PlaceTable`/`PlaceJudge`/`DiscardPile` 统一）后加入可见牌列表（`pile.cardItems`）；超过可见数（`numCardsVisible`）时 `_markOverflowClearance` 给最老未标记牌设清除时间戳（`clearTimestamps[cardId]`）；Timer 每秒 `currentTime`+1，超时（`currentTime - ts > sClearanceDelayBuckets(3)`）的牌直接 `destroy()` + 从列表移除 + `lay()` 重排（不淡出；旧版 `TablePile.cpp` `_fadeOutCardsLocked` 有 opacity→0 淡出，QML 暂不做）。`PlaceTable`/`PlaceJudge`/`DiscardPile` 三者间互转（TablePile 内部 place 变化）跳过 add/remove，牌原位保留（不改顺序、不重启动画）。进出场动画（新建+销毁，无源位置飞行）待"CardItem 移动动画"任务统一处理。
  - **不需要 cardPool**：没有"空闲暂存"状态——容器间移动直接到目标容器（`lay()` + `goBack()`）；无目标的牌 `destroy()`。牌按需动态创建/销毁，非静态预创建。
  - 容器销毁时（`Component.onDestruction`）：把其持有的 CardItem visual parent 重置回 roomScene，由 roomScene 统一管理后续（继续移动或 destroy）。QObject parent 永远是 roomScene，容器销毁不动 QObject 生命周期，安全。
- **CardContainer.qml 现有 `createItem` 需改造（涉及转移问题）**：`qml/CardContainer.qml:10` 的 `createItem(cardId)` 当前用 `createObject(this)`（QObject parent = CardContainer），**牌移到其他容器时 QObject parent 还在 CardContainer，涉及转移问题**。必须改为 `createObject(roomScene)`（QObject parent = roomScene）+ `cardItem.parent = this`（visual parent = CardContainer）。这样原实例移动只切 visual parent，不动 QObject parent。
- **旧版对象移动模式沿用精神**：旧版 C++ `RoomScene::_m_cardsMoveStash`(`uibackup/roomscene.cpp:2080`) 的 `from_container->removeCardItems` → `to_container->addCardItems` 实例转移 + `setParentItem` 切 visual。QML 沿用"原实例移动"精神，用 `cardItem.parent = xxx`（QQuickItem visual parent）替代 `setParentItem`，QObject parent 固定 roomScene 替代 C++ 自由 reparent。

### 装备区与技能按钮（旧版实现参考）
- **装备区基类机制**（`GenericCardContainerUI.h:272-276`）：5 槽统一在 `PlayerCardContainer` 基类管理（`_m_equipRegions[5]` QGraphicsProxyWidget + `_m_equipCards[5]` CardItem + `_m_equipLabel[5]` QLabel + `_m_equipAnim[5]`）。槽位索引 = `EquipCard::location()`（武器0/防具1/防马2/攻马3/宝物4）。纯虚 `_getEquipParent()` 分流：Dashboard→`_m_leftFrame`（左侧），Photo→`_m_groupMain`（主体下方）。`addEquips`/`removeEquips`/`_updateEquips` 基类实现，self/非self 共用。
- **broken 装备占位图**：`_getEquipPixmap`(`GenericCardContainerUI.cpp:968-978`) 按 `Player::isBrokenEquip(id)` 选 `S_SKIN_KEY_EQUIP_ICON`（正常）或 `S_SKIN_KEY_EQUIP_BROKEN_ICON`（占位图）。`broken_equips`(`player.h:389`) 是 QList<int>；`isBrokenEquip(id, consider_shenbao)`(`player.cpp:197`) 含"神豹"技能特判。通知链：服务器 `S_COMMAND_SET_BROKEN_EQUIP` → `Client::setBrokenEquips`(`client.cpp:251`) → `Player::setBrokenEquips` → `brokenEquips_changed` → `updateBrokenEquips`→`_updateEquips` 重绘。**走 Player 信号，非 Client notify 信号**。`getWeapon`/`getArmor`/`getTreasure`(`player.cpp:1097/1113/1129`) 对 broken 装备返回 nullptr（逻辑上失效）。
- **装备点击分流（双重功能）**：`Dashboard::mouseReleaseEvent`(`dashboard.cpp:770-794`) 统一处理装备槽点击——遍历 `_m_equipRegions[i]->isUnderMouse()` 找命中槽；**优先级**：`_m_equipSkillBtns[i]` 存在且 enabled → `click()` 发动装备技能；否则 `to_select->isMarkable()` → `mark()` 选中。Q_ASSERT 保证装备技能按钮 isDown 时不会走到 mark（"不能在发动装备技能时选该装备当牌"）。**同一点击入口按状态分流**，非两个独立可点击层。
- **装备技能按钮不挂场景（视觉靠边框动画）**：`_m_equipSkillBtns[5]`(`dashboard.h:288`) 创建时 `new QSanInvokeSkillButton()`（**无 parent 参数**，对比 `addSkillButtonByName` 的 `new QSanInvokeSkillButton(this)`），不挂任何 item/场景，故不显示。视觉反馈靠 `_m_equipBorders[i]`(`dashboard.h:287`) 边框动画（`image/system/emotion/equipborder/`，`_onEquipSelectChanged`→`_setEquipBorderAnimation`）；点击由 `Dashboard::mouseReleaseEvent` 主动 `click()`。**QSanInvokeSkillButton 本身 `_repaint`/`paint` 是完整实现**（`qsanbutton.cpp:377-477`：`getSkillButtonPixmap` 皮肤图+技能名文字+ATTACHEDLORD 武将头像+`isSkillInvalid` 红叉+CANPRESHOW 黄框），武将技能按钮经 `dock.addSkillButtonByName` 挂 dock 显示，装备技能按钮因不挂场景而不显示。装备技能来源 `Sanguosha->getSkill(equip)`，`equip_skill` 标志（`skill.cpp:231`，装备技能构造时设 true）。
- **技能按钮三类 + addSkillButton 分流**：Dashboard 持 `_m_skillDock`(主将)/`_m_rightSkillDock`(副将，国战)/`_m_equipSkillBtns[5]`(装备槽)。`addSkillButton`(`dashboard.cpp:589-638`) 先遍历 5 装备槽匹配 skillName（`Sanguosha->getSkill(equip)->objectName()`），命中→建装备技能按钮 return；否则进 head?`_m_skillDock`:`_m_rightSkillDock`。**装备技能优先匹配装备槽，不进 dock**。
- **技能按钮视觉与 dock 布局**（`qsanbutton.cpp`）：`QSanInvokeSkillButton::_repaint`(377-404) 填 `_m_bgPixmap[i] = G_ROOM_SKIN.getSkillButtonPixmap(state, skillType, width)` + CANPRESHOW 半透明 + `getSkillTextFont` 画技能名（非 WIDE 取前 2 字）；`paint`(406-477) `drawPixmap` + ATTACHEDLORD 画武将头像（找拥有该技能的武将 `generalName.png`）+ `isSkillInvalid` 红叉 + CANPRESHOW 黄框。`QSanInvokeSkillDock::update`(508-610) 布局：regular_buttons + lordskill_buttons 分组，按行排列（每行最多 3，末行 1 个时平衡到 2），`setButtonWidth`/`setPos`，lordskill 单独定位（左侧偏移），`m_skillButtonSank` 影响行偏移。`QSanSkillButton::onMouseClick`(242-262) 触发：CANPRESHOW→`preshow`；TOGGLE+isDown+`_m_emitActivateSignal`→`skill_activated`，!isDown+`_m_emitDeactivateSignal`→`skill_deactivated`。
- **技能按钮驱动**（`roomscene.cpp` 桥接）：Client 信号→notify：`skill_attached`→`notifySkillAttached`；`skill_detached`→`notifySkillDetached`；`skill_acquired`→`notifySkillAcquired`（游戏中获得技能，如转移/觉醒）；`skill_invalidity_changed`→`notifySkillInvalidityChanged`；**初始技能（武将选择后）走游戏事件 `S_GAME_EVENT_ADD_SKILL`**（`Client::event_received`→`notifyEventReceived`），**非 `skill_acquired`**。QML skillDock 监听 `notifyEventReceived`（及 skill_attached/detached/acquired/invalidity_changed）按事件类型分发：带 Skill 的 6 个事件（ADD/LOSE/ACQUIRE/DETACH/PREPARE/UPDATE → `rebuild`，`getPlayerSkillButtons(roomScene.Self)`）；`SKILL_INVOKED` → 倒计时 TODO。事件类型用 `QSanProtocol.S_GAME_EVENT_*` 枚举比较（`Q_NAMESPACE` + `Q_ENUM_NS` 暴露，见目录结构小节）。**分发结构**：RoomScene 顶层 `onNotifyEventReceived`（全局游戏事件分发中枢）按事件类型转发 skill 相关 → `skillDock.handleSkillEvent`（`qml/SkillDock.qml` 拆分组件，含 `buttonWidths`/`handleSkillEvent`/`rebuild`/`Flow`+`Repeater`/skill 专属 Connections）；skill 专属 notify（attached/detached/acquired/invalidity_changed）由 SkillDock.qml 内部 Connections 监听 → `rebuild`（skill 专属信号非全局事件，放 SkillDock 内）。旧版 `roomscene::addSkillButton`(`roomscene.cpp:2397-2428`) 连接 viewAsSkill→`skill_activated`→`skillButtonActivated`/`onSkillActivated`，skill 有 dialog→`dialog.popup`（QML 版 pending/dialog TODO）。**注**：`add_equip_skill`/`remove_equip_skill` 信号（`GenericCardContainerUI.cpp:1082/1122` emit）在 uibackup 无 connect（死代码），装备技能按钮实际由 `skill_attached`/`skill_acquired` 驱动（`addSkillButton` 遍历装备槽匹配），不由装备增删直接驱动。
- **ViewAsSkill 用装备发动**：`startPending`(`dashboard.cpp:1200-1248`) 连接 `_m_equipCards[i]` mark_changed→`onMarkChanged`；`updatePending`(`dashboard.cpp:1600-1610`) 装备按 `view_as_skill->viewFilter(pended, equip->getCard())` 设 markable，不可 mark 且装备技能按钮不可用→`_m_equipRegions[i]` opacity 0.7；`onMarkChanged`(`dashboard.cpp:1694-1712`) 装备 mark→加入 pendings→updatePending。装备技能本身可能是 ViewAsSkill（`ViewAsSkill::parseViewAsSkill` flatten，`qsanbutton.cpp:270`），点击装备技能按钮→`skill_activated`→`startPending`。
- **互斥**（`dashboard.cpp:708-741`）：`skillButtonActivated`——激活任一技能按钮（sender）→dock 其他 isDown 的 ViewAsSkill 按钮重置 UP + 其他装备技能按钮 `setEnabled(false)`；`skillButtonDeactivated`→恢复其他装备技能按钮 `setEnabled(true)`，若 isDown 则 `click()` 取消。同一时间只一个 ViewAsSkill 激活。
- **chooseSkillButton**（`roomscene.cpp:3936-3962`）：`AskForSkillInvoke` 时若有多个 enabled 技能按钮，弹 QDialog 列表双击选择（`btn->click()`）。
- **Photo（非 self）装备**：`photo.cpp:199-219` `_addCardItems`：PlaceEquip→`addEquips`（基类），只展示（QLabel pixmap），**无选中态、无技能按钮、无 pending**。broken 占位图同样适用（`_getEquipPixmap` 基类共用）。
- **武将技能按钮 dock 位置 + 多状态**：旧版 `_m_skillDock`/`_m_rightSkillDock`(`dashboard.cpp:265-279`) 是双 dock（主将/副将），parent=`_m_rightFrame`（dashboard 右侧=self 头像区），pos 在 avatar 下方。**QML 版不做双 dock**——Dashboard 已去除武将图部分改用 selfPhoto，双 dock 会违和；武将技能按钮进**单 dock**，锚定 selfPhoto（底部对齐、向上延伸）、parent = RoomScene。武将技能按钮与装备技能按钮同类（`QSanInvokeSkillButton`），多状态：SkillType 6 种（`setSkill` 按 skill 类型判定：BattleArray→ARRAY、isWake→AWAKEN、isLimited→ONEOFF_SPELL/ATTACHEDLORD、isFrequent/optional_trigger→FREQUENT、isCompulsory/Eternal/passive_modifier→COMPULSORY、else→PROACTIVE/ATTACHEDLORD，`qsanbutton.cpp:279-338`）× ButtonState 5 种（UP/HOVER/DOWN/CANPRESHOW/DISABLED）。`setState` refine（COMPULSORY+hasShownSkill→DISABLED，国战 canPreshow→CANPRESHOW）。视觉：按 SkillType+ButtonState 选皮肤图+画技能名（`getSkillButtonPixmap`，`_repaint`/`paint` 完整实现见 `qsanbutton.cpp:377-477`）。**attachlord 单独 dock、武将图显示等留 TODO 后续追加**。
- **QML 装备区拆两套（不共用）**：旧版 C++ 用 `PlayerCardContainer` 基类 + `_getEquipParent()` 分流（Dashboard/Photo 共用 `addEquips` 等逻辑）。QML 不沿用基类共用——**拆两套**：self 装备区进 Dashboard 左侧（`SelfEquipArea.qml` 或 Dashboard 内联，含点击分流/有效态/broken）；非 self 装备区独立 `PhotoEquipArea.qml`，Photo 用 `Component.createObject` 加载放 Photo 下方（纯展示+broken）。可共用 `EquipAreaBase.qml` 基类（5 槽布局+装备图+broken 占位图），派生差异化。原因：self/非self 交互差异大（选中态/技能按钮/pending 仅 self 有），共用单一组件需大量 selfPhoto 分支，不如派生清晰。

### 其他
- **qmllint 假告警**：未生成 qmltypes 时 `CppRoomScene`/`rocks.touhousatsu` 未识别，大量 `unqualified`/`missing-type` warning，构建后消除，非真实错误。
- **auto 使用规则**（C++ modernize 约束）：**`auto` 仅用于无法显式写出类型的 lambda 相关场景**——(1) 声明 lambda 对象本身（`auto f = [](){};`）；(2) 模板推导结果含 lambda 类型（lambda 类型匿名，只能用 auto，如 `auto x = std::make_tuple([](){}, 1);`）。其余一律用显式类型——含 lambda 函数体内、普通变量、范围 for、返回值。**禁用结构化绑定 `auto [a,b,c] = ...`**。另：C++17 CTAD `std::unique_ptr ptr = std::make_unique<xxx>();`（不写模板参数，靠推导）可用。
- **日志**：桥接层 `qDebug` 带 `[bridge]` 前缀，不打印大 payload。
- **旧代码类名注意**：`src/uibackup/` 中以 `Q+大写字母` 打头的类不全是 Qt 自带，有自定义类：`QSan*` 系列（`QSanSelectableItem`/`QSanButton`/`QSanSkillButton`/`QSanInvokeSkillButton` 等）与 `QAnimatedEffect`(`src/uibackup/sprite.h:28`，继承 Qt 自带 `QGraphicsEffect`，用于动画效果，配合 `EffectAnimation` 使用)。阅读代码时需确认是 Qt 自带（如 `QGraphicsDropShadowEffect`/`QGraphicsObject`/`QGraphicsProxyWidget`/`QGraphicsEffect`）还是自定义（`QSan*` 系列、`QAnimatedEffect`）。
- **旧代码宏定义注意**：`G_ROOM_SKIN`/`G_DASHBOARD_LAYOUT`/`G_ROOM_LAYOUT`/`G_PHOTO_LAYOUT`/`G_COMMON_LAYOUT` 等是宏定义（`#define`，在 `SkinBank.h:481-485`，**非全局变量**），旧 `uibackup` 代码大量使用（如 `G_COMMON_LAYOUT.m_cardNormalHeight`、`G_ROOM_LAYOUT.m_discardPilePadding` 等）。`SkinBank.h`/`SkinBank.cpp` 在 `uibackup/`（含 `QSanRoomSkin` 完整定义：`PlayerCardContainerLayout`/`DashboardLayout`/`PhotoLayout` 结构、`getSkillButtonPixmap`/`getButtonPixmap` 等、`S_SKIN_KEY_EQUIP_ICON`/`S_SKIN_KEY_EQUIP_BROKEN_ICON` 等皮肤键）；`src/dialog/uilegacy/` 也有一份（仍在用，非死代码）。阅读旧代码时 `G_*` 形式的标识符需确认是宏还是变量。
- **兼容性**：保留 `MainWindow` 现有 `qml_switchToRoomScene` 等接口签名。
- **QTBUG-147713 workaround**（`property list<xxx>` 导致 qmlformat 解析错乱）：workaround 是加一个字母序靠后的 property——RoomScene.qml 用 `readonly property int zzzWorkaroundQTBUG147713: 0`（`zzz` 前缀确保排在 `otherPhotos` 后）。**删除条件**：之后若新增 property 字母序排在 `list<xxx>` 类型 property 之后，该 property 自然承担"排序靠后"角色，即可删除 `zzzWorkaroundQTBUG147713`。
- **QTBUG-148521 workaround**（qmlformat 6.11.1：函数 `: var` 返回类型注解 + `.qmlformat.ini` 设 `MaxColumnWidth` → 函数体缩进错乱 + 末尾多 `}`；`:string` 返回类型不触发，仅 `: var` 触发）：`RoomScene.qml` `findPhotoByPlayerName` 暂不加 `: var`（带 `// TODO: add : var once QTBUG-148521 is resolved` 注释），忍受运行时 "insufficiently annotated" 警告（`target.addEquip`/`removeEquip` 调用）。**删除条件**：QTBUG-148521 修复后加回 `: var`。

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
- `engine.h`：`Engine::getGeneral` 加 `Q_INVOKABLE`，QML 直接 `Sanguosha.getGeneral(name).kingdom` 读势力，不走 RoomScene 桥接。
- `ChooseGeneralBox.qml`：`_canPair(g1, g2)` 直接 `Sanguosha.getGeneral(name).kingdom`（对齐 `room.cpp:3723`/旧版 `choosegeneralbox.cpp:489`，不判空不过度防御）；`_toggle` 双将模式选第二个将时校验，不合规拒绝；不合规候选灰显。

### 2026-07-19：注册 General 到 QML
- `qmlui.cpp`：`registerCore` 新增 `qmlRegisterUncreatableType<General>`（紧随 ClientPlayer），include `general.h`。
- 目的：`Engine::getGeneral` 加 `Q_INVOKABLE` 后 QML 拿到 `const General *`，注册 General 类型使 QML/qmllint 能识别并读其 `Q_PROPERTY`（`kingdom`/`maxhp`/`gender`/`lord`/`hidden` 等），后续可复用，不再依赖未注册类型的元对象回退。

### 2026-07-19：确立"禁用 z 属性"约定
- UI 约定新增：所有 UI 靠声明顺序/父子层级解决覆盖，不用 `z`。
- CardItem 灰显遮罩移除 `z: 1`（靠在 cardContent/MouseArea 之后声明保证上层覆盖）。

### 2026-07-19：CardItem 禁用机制改用标准 enabled
- 灰显从自定义 `dimmed` 属性改为 Qt 标准 `enabled` 属性联动：`enabled: !_isDimmed(g)`，遮罩 `visible: !enabled`。
- 原因：CardItem 作为手牌/装备等其他牌时也需禁用情况，统一用 `enabled` 复用一套机制（禁用时 MouseArea 自动不响应 + 遮罩变暗）。

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

### 2026-07-20：Dashboard 按钮布局改旧版 platter 集合
- 4 按钮（OK/Cancel/Discard/Trust）从水平 Row 改为 **platter 按钮集合**（对标旧版 `roomscene.cpp:800-815` + `skins/defaultSkin.layout.json`）。
- `qml/QSanButton.qml`：加 `property url normalSource`/`hoverSource`/`downSource`/`disabledSource`（调用方指定各状态图，QSanButton 按 state 选取，空 fallback `normalSource`）+ `property bool overlayEnabled`（hover 叠加与 Text 显隐）。**调用方直接指定各状态 image，不拼接路径、不约定文件名**：platter 按钮 4 个 source 都设 + `overlayEnabled: false`；普通按钮只设 `normalSource`。
- `qml/Dashboard.qml`：删 Row，加 `buttonSet`（`Item` 100×195，`bg.png` 背景 + 4 platter 按钮按 `confirmButtonArea` [6,10,75,90] / `cancelButtonArea` [1,112,75,90] / `discardButtonArea` [67,60,77,95] / `trustButtonArea` [62,162,70,70] 叠放）；`buttonSet` 定位 `cardBg` 右上（`anchors.right/top`）；`cardArea` `rightMargin: 120` 让出 buttonSet 空间。trust 按钮保留 `checkable: true`（旧版 S_STYLE_TOGGLE）。
- 其他 11 处 QSanButton 用法（RoomScene 3 + StartScene 8）`source` → `directSource`。

### 2026-07-20：A-手牌区同步实现
- **桥接层**：`roomscene.h` 加 `notifyMoveCardsGot`/`notifyMoveCardsLost` 信号（携 `moveId` + `QVariantList moves`）；`roomscene.cpp` `connectClientSignals()` 连接 `Client::move_cards_got`/`move_cards_lost`，lambda 遍历 `QList<CardsMoveStruct>` 转 `QVariantList<QVariantMap>`（字段：`cardIds`/`fromPlace`/`toPlace`/`fromPlayer`(`Player *`)/`toPlayer`(`Player *`)/`fromPileName`/`toPileName`），`from`/`to` 直接用结构体指针（`getCards`/`loseCards` 已 `getPlayer` 补设，非 null），加 `#include "structs.h"`。
- **CardContainer.qml**：加 `id: cardContainer` + `property var rootScene`；`createItem` 改 `createObject(rootScene)` + `item.parent = cardContainer`（QObject parent = roomScene，visual parent = cardContainer，见"CardItem 与牌容器设计"小节）；加 `removeItem(cardId)`（按 cardId 找 + destroy）。
- **Dashboard.qml**：加 `property var roomScene` + `addHandCard(cardId)`/`removeHandCard(cardId)` 函数（调 `cardArea.createItem`/`removeItem` + `lay(Qt.AlignLeft, 1, 0, true, true)`）；CardContainer 设 `rootScene: dashboard.roomScene`。
- **RoomScene.qml**：Dashboard 实例设 `roomScene: roomScene`；`onNotifyMoveCardsGot`/`onNotifyMoveCardsLost` 处理 `toPlace`/`fromPlace == Player.PlaceHand && player.objectName == Self.objectName` 的 move，遍历 `cardIds` 调 `dashboard.addHandCard`/`removeHandCard`。
- **CardItem.qml**：`onCardIdChanged` 的 `cardId == -1` 分支补全牌背显示（`cardImage.source = card-back.png` + 隐藏花色/点数）。

### 2026-07-20：PhaseItem 构造模式 + selfPhoto 标识 + QSanButton hover
- 确立 **selfPhoto vs 非 selfPhoto 构造模式**（见"UI 约定"小节）：selfPhoto 不构造的子组件由 Dashboard 承担显示，非 selfPhoto 用 `createObject` 动态构造。
- `qml/Photo.qml`：`selfPhoto` 改 `required property bool selfPhoto`（强制显式传值）；`PhaseItem` 从静态声明 + `visible: !selfPhoto` 改为 `createPhaseItem()` 命令式创建（`Component.createObject`，`Component.onCompleted` 里 `if (!selfPhoto)` 调用——selfPhoto 不构造；无销毁，`visible` 绑 `gameStarted`）——避开 `Loader` 的 `pragma ComponentBehavior: Bound` 告警（Component 模板 `PhaseItem {}` 内不访问外层 id，`Qt.binding(() => photo.xxx)` 闭包绑 anchors/phase/visible）。
- `qml/RoomScene.qml`：selfPhoto 实例设 `selfPhoto: true`；占位 Photo（`Component.onCompleted` 创建）与测试桩 Photo 显式 `selfPhoto: false`。
- `qml/QSanButton.qml`：hover 状态加 `hover.color`（`entered`/`downEntered`/`downExited` 用 `Qt.rgba(1,1,1,.25)` 白色半透明，`disabled` 用 `Qt.rgba(0,0,0,.25)` 黑色半透明）。**加 platter 皮肤支持**：`buttonSkin`/`buttonName` 属性，非空时 source 按 state 自动切换 `image/system/button/<buttonSkin>/<buttonName>/<normal|hover|down|disabled>.png`；空时用 `directSource`（原 `source` 改名，现有非 platter 用法已迁移）。platter 模式隐藏 hover 叠加与 Text（platter 图自带 hover 态与图标）。
- 后续装备区同理：selfPhoto 在 Dashboard 构造，非 selfPhoto 在 Photo 用 `createObject` 动态构造。

### 2026-07-19：Photo 化身图绑定化 + RoomScene 测试桩显隐
- `Photo.qml`：化身图 `huashenImage`/`huashen2Image` 改声明式绑定（`source: getImageSourceUrl(huashenGeneral)`、`visible: huashenGeneral != ""`），`_hegemony` 后缀 fallback 由 `getImageSourceUrl` 内部 `assetExists` 统一处理；删除 `onHuashenGeneralChanged`/`onHuashenGeneral2Changed` 命令式处理器（含手写 `_hegemony` 截断逻辑，现由 `getImageSourceUrl` 覆盖）。化身图 opacity 循环动画（500ms 淡入→4s 停→500ms 淡出→1s 间歇）由 `onVisibleChanged` 启停。
- `RoomScene.qml`：`testItemToBeRemovedAfterTest` 的 `visible` 从 `true` 改为 `!gameStarted`（游戏开始后隐藏测试桩）。
- RoomScene 的"返回主菜单"按钮 `startSceneButton`（`visible: !gameStarted && !gameOver`，调 `MainWindowInstance.gotoStartScene()`）。

### 2026-07-21：装备区与技能按钮旧版实现调研
- broken 装备视觉：`_getEquipPixmap` 按 isBrokenEquip 选占位图 `S_SKIN_KEY_EQUIP_BROKEN_ICON`；brokenEquips 通知走 Player 信号（服务器 `S_COMMAND_SET_BROKEN_EQUIP`→`Client::setBrokenEquips`→`Player::setBrokenEquips`→`brokenEquips_changed`）。
- 装备双重功能：`Dashboard::mouseReleaseEvent`(`dashboard.cpp:770-794`) 统一分流装备槽点击——装备技能按钮 enabled→发动技能，否则 markable→mark 选中（同一点击入口按状态分流）。
- 装备技能按钮 `_m_equipSkillBtns[i] = new QSanInvokeSkillButton()` 无 parent 不挂场景，不显示自身，视觉靠 `_m_equipBorders` 边框动画。
- ViewAsSkill 用装备发动 + 互斥：`startPending` 连装备 mark_changed；`updatePending` 按 viewFilter 判装备 markable；`skillButtonActivated` 互斥（dock 其他 ViewAsSkill 重置 + 其他装备技能按钮禁用）。
- 成果补入"装备区与技能按钮（旧版实现参考）"小节 + 扩展"Dashboard 技能按钮 + 装备区"任务条目子任务拆分。

### 2026-07-21：装备区拆两套 + 武将技能按钮位置确认
- 确认武将技能按钮 dock 位置：`_m_skillDock`/`_m_rightSkillDock`(`dashboard.cpp:265-279`) parent=`_m_rightFrame`（dashboard 右侧=self 头像区），pos 在 avatar 下方——**视觉在 self 头像下方，parent 是 dashboard**。武将技能按钮与装备技能按钮同类 `QSanInvokeSkillButton`，多状态（SkillType 6 × ButtonState 5，`setSkill` 按 skill 类型判定，`qsanbutton.cpp:263-348`）。
- 决策：**装备区 QML 拆两套**（不沿用旧版基类共用）——self 进 Dashboard 左侧（`SelfEquipArea.qml`/内联，含点击分流/有效态/broken）；非 self 独立 `PhotoEquipArea.qml`，Photo `Component.createObject` 加载放 Photo 下方（纯展示+broken）。共用 `EquipAreaBase.qml` 基类派生。原因：self/非self 交互差异大，共用单组件需大量分支，派生更清晰。
- 更新任务条目：装备区子任务改为拆两套；QSanSkillButton 子任务补充武将技能按钮位置（selfPhoto 下方 parent=dashboard）+ 多状态；设计小节新增"武将技能按钮 dock 位置 + 多状态"与"QML 装备区拆两套"两条。

### 2026-07-21：武将技能按钮改单 dock
- 决策：**武将技能按钮不做主将/副将双 dock**——QML 版 Dashboard 已去除武将图部分改用 selfPhoto，旧版双 dock（主将 dock 在 self 头像下方 + 副将 dock 在副头像下方）会违和。武将技能按钮进单 dock（视觉放 selfPhoto 下方，parent = Dashboard）。
- attachlord 单独 dock、武将图显示等旧版有但 QML 暂不做的功能，留 TODO 后续追加（不在代码留 TODO，仅 plan 记录）。
- 更新 QSanSkillButton 子任务（三类改两类：装备技能按钮 + 武将技能单 dock）+ 设计小节"武将技能按钮 dock 位置 + 多状态"条目。

### 2026-07-21：技能按钮组件 + 桥接
- C++ 桥接 `roomscene.h/cpp`：加 `Q_INVOKABLE QVariantList getPlayerSkillButtons(ClientPlayer *player) const` 返回任意 player 可见非装备技能列表（QVariantMap：`skillName`/`skillType`/`translatedName`/`description`/`viewAsSkillName`），self dock + 其他 player tooltip 共用；`getSelfSkillButtons()` 是 `getPlayerSkillButtons(Self)` 便捷封装；`skillTypeString()` 辅助函数对标旧 `setSkill`(`qsanbutton.cpp:279-338`) 判定 7 种 SkillType；LordSkill 过滤按该 player 是否主公。include `engine.h`/`skill.h`。skill_attached/detached/acquired/invalidity_changed 信号转发已有（`roomscene.cpp:239-249`）。qmlui 代码 modernize C++17（`[[nodiscard]]` getter、`u"..."_s` 字面量替代 `QStringLiteral`；`auto` 仅限 lambda，见"其他"小节）。
- `qml/QSanSkillButton.qml`（新）：多状态图片按钮，source = `image/system/button/skill/<skillType>/<buttonWidth>-<state>.png`（7 skillType × 3 宽度 × normal/hover/down/disabled）；toggleable 类型（proactive/oneoff/array/attachedlord）点击 toggle + emit `skillActivated`/`skillDeactivated`；非 toggleable（compulsory/awaken/frequent）仅显示；文字用 `G.SkillButtonFontFace`，非 wide 取前 2 字。
- `qml/RoomScene.qml`：加技能按钮单 dock（`skillDock`），`anchors.bottom: selfPhoto.bottom` + `anchors.left`/`right: selfPhoto`（底部对齐 selfPhoto、向上延伸），`Flow`+`Repeater` 布局，`buttonWidths(count)` 按旧版 `dock.update()`(`qsanbutton.cpp:508-610`) 行排列+末行平衡生成每按钮宽度（1→[1]；2→[2,2]；3→[2,2,1]；3n+1→(n-1)*3 个 3+4 个 2；3n+2→n*3 个 3+2 个 2；3n+3→全 3），rebuild 时缓存数组，Repeater 按索引取；`Component.onCompleted` 调 `rebuild()`；`Connections` 监听 `onNotifySkillAttached/Detached/Acquired/InvalidityChanged` 全量重建。
- `.pro` OTHER_FILES 加 `qml/QSanSkillButton.qml`。
- TODO：ViewAsSkill pending + 互斥 + chooseSkillButton（见"待做"小节）；attachlord 武将头像、国战 canPreshow 视觉；Photo tooltip 显示其他 player 技能（`getPlayerSkillButtons`），tooltip 后续精致化。

### 2026-07-22：Client::handleGameEvent 补技能状态更新
- 旧版代码中 `Client::handleGameEvent` 未调用 `player->addSkill`/`loseSkill`/`acquireSkill`/`detachSkill`（`skills` map 由 `addSkill` 填充，`getPlayerSkillButtons` 依赖它）。本次因初始技能显示需要（武将选择后 `S_GAME_EVENT_ADD_SKILL` 需填 `skills`），在 `Client::handleGameEvent`(`client.cpp:330`) 加入技能 4 事件（ADD/LOSE/ACQUIRE/DETACH SKILL）的 player 状态更新，UI 无关逻辑从旧 `RoomScene::handleGameEvent` 移入。`emit event_received` 后桥接转发 `notifyEventReceived` 驱动 QML。其余 UI 无关事件（preshow/gender/hero）TODO。

### 2026-07-22：QSanProtocol 枚举暴露 + notifyEventReceived 按 Skill 事件分发
- `src/core/protocol.h`：`QSanProtocol` namespace 加 `Q_NAMESPACE`，`GameEventType` 后加 `Q_ENUM_NS(GameEventType)`（暴露所有 `S_GAME_EVENT_*` 枚举给 QML）。`qmlui.cpp` `registerCore` 加 `qmlRegisterUncreatableMetaObject(QSanProtocol::staticMetaObject, "rocks.touhousatsu", 1, 0, "QSanProtocol", ...)` 注册，QML 用 `QSanProtocol.S_GAME_EVENT_ADD_SKILL` 等访问（需 qmake 重新生成 Makefile 让 moc 处理 protocol.h）。
- `qml/RoomScene.qml` skillDock：`handleSkillEvent(args)` 按 `args[0]` 事件类型分发——ADD/LOSE/ACQUIRE/DETACH/PREPARE/UPDATE SKILL → `rebuild`；`SKILL_INVOKED` → 倒计时 TODO。`onNotifyEventReceived` 调 `handleSkillEvent`（不再无条件 rebuild）。

### 2026-07-22：SkillDock.qml 拆分 + 全局事件分发中枢
- 拆 `qml/SkillDock.qml`（原 RoomScene.qml 内联 skillDock）：含 `property var roomScene`、`buttonWidths`/`handleSkillEvent`/`rebuild`、`Flow`+`Repeater`+`QSanSkillButton`、skill 专属 Connections（onNotifySkillAcquired/Attached/Detached/InvalidityChanged → `rebuild`）。RoomScene 实例化 `SkillDock { id: skillDock; roomScene: roomScene; anchors.bottom: selfPhoto.bottom; anchors.left: selfPhoto.left; anchors.right: selfPhoto.right }`。
- `notifyEventReceived`（全局游戏事件流）移到 RoomScene 顶层 Connections 的 `onNotifyEventReceived`（全局事件分发中枢，按事件类型转发——当前 skill 相关 → `skillDock.handleSkillEvent`，其他事件类型 TODO），不再放 SkillDock 内（全局状态非 SkillDock 私有）。skill 专属 notify 留 SkillDock 内部。
- `.pro` OTHER_FILES 加 `qml/SkillDock.qml`。

### 2026-07-22：A-装备区同步实现
- **桥接层**：`roomscene.cpp` `move_cards_got`/`lost` lambda 转 `qmlMove`（cardIds/fromPlace/toPlace/fromPlayer/toPlayer/fromPileName/toPileName），**不解析装备槽位**——槽位由 QML 读 `card.getRealCard().location`（getEngineCard 返回 WrappedCard，须 getRealCard 拿真实 EquipCard）获取。
- **Dashboard.qml**：`equipBg` 内 5 个占位 `Image` 替换为 `SelfEquipArea` 实例（id `equipArea`，anchors topMargin 72）；`property alias equipArea` 暴露给 RoomScene。`equipCardIds`/`addEquip`/`removeEquip` 移入 `EquipAreaBase`（Dashboard 不再持有）。
- **Photo.qml**：非 self 装备区改为 `PhotoEquipArea` 实例（id `equipArea`，`anchors.bottom: photo.bottom` 叠加内部底部，`visible: !selfPhoto`）；`property alias equipArea` 暴露。`equipCardIds`/`addEquip`/`removeEquip` 移入 `EquipAreaBase`。
- **RoomScene.qml**：加 `findPhotoByPlayerName(playerName)` 辅助函数；`onNotifyMoveCardsGot`/`Lost` 增加 `PlaceEquip` 分支——按 `toPlayer`/`fromPlayer`（self → `dashboard.equipArea`，其他 → `findPhotoByPlayerName(...).equipArea`）分发，遍历 `cardIds`，`var loc = Sanguosha.getEngineCard(m.cardIds[k]).getRealCard().location` 读槽位（getEngineCard 返回 WrappedCard，须 getRealCard 拿真实 EquipCard 才有 location Q_PROPERTY），调 `target.addEquip(loc, cardId)`/`removeEquip(loc)`。
- **EquipSlot.qml**（新）：单槽显示，`property int cardId`，`onCardIdChanged` 经 `Sanguosha.getEngineCard(cardId)` 取卡读 `objectName`/`suit`/`number_string`/`red`（基类属性，WrappedCard 透传，不需 getRealCard）。**可配置**（`equipIconDir`/`equipIconWidth`/`equipIconHeight`/`suitArea`/`pointArea`）：Dashboard 默认 `image/equips/` 149×25（`suitArea [128,5,21,17]`/`pointArea [117,-5,30,30]`），Photo 设 `image/fullskin/small-equips/` 140×19（`suitArea [117,2,21,17]`/`pointArea [106,-4,25,25]`）。对标旧 `_getEquipPixmap` + defaultSkin layout（dashboard/photo 两段，`equipTextArea`/`equipDistanceArea` = `[0,0,0,0]`，**图标自带装备名**，无装备名/距离 text）。`visible` 绑 `cardId !== -1`。
- **EquipAreaBase.qml**/`SelfEquipArea.qml`/`PhotoEquipArea.qml`（新，装备区拆两套实现）：`EquipAreaBase` 基类（5 槽 Repeater + `equipCardIds`/`addEquip`/`removeEquip` + 可配置图标/布局，动画预留 `Repeater.itemAt`）；`SelfEquipArea`（EquipAreaBase 根，dashboard 配置，进 Dashboard `equipBg`）；`PhotoEquipArea`（EquipAreaBase 根，small-equips 配置，叠加 Photo 底部）。Dashboard/Photo 用 `property alias equipArea` 暴露，`equipCardIds`/`addEquip`/`removeEquip` 从 Dashboard/Photo 移入 EquipAreaBase。RoomScene 调 `dashboard.equipArea`/`photo.equipArea`。
- **数据流**：`Client::getCards`→`_getSingleCard`→`ClientPlayer::addCard(PlaceEquip)`→`setEquip`（Client 内部更新 player 装备指针，不发信号）；桥接 `move_cards_got` 转 move 字段传 QML，QML 侧读 `card.getRealCard().location` 得槽位，`addEquip`/`removeEquip` 维护 `equipCardIds` 数组驱动 `EquipSlot` 显示。**不用 Player 装备 Q_PROPERTY 绑定**（setEquip/removeEquip 无信号，且与 A 子任务 move 驱动设计一致）。
- TODO：broken 占位图、距离文字（武器 range/马 correct）、装备点击分流、有效态、装备技能按钮留待"Dashboard 技能按钮 + 装备区"任务；Photo 装备区后续拆 `PhotoEquipArea` + 动态构造（与 PhaseItem 模式统一）。

### 2026-07-22：EquipCard 枚举暴露给 QML
- `src/package/standard.h`：`EquipCard` 的 `Q_ENUMS(Location)` 改 `Q_ENUM(Location)`（须在 enum 声明后）；加 `Q_PROPERTY(Location location READ location STORED false)`（QML 读 `card.location` 直接得槽位）。`Weapon` 加 `Q_PROPERTY(int range READ getRange)`（距离显示用，A 子任务暂不用）。
- `src/qmlui/qmlui.cpp`：`registerCore` 加 `qmlRegisterUncreatableType<EquipCard>`（紧随 Card），`#include "standard.h"`；EquipCard 是抽象类（`virtual location()=0`），注册仅暴露 `Location` 枚举（`WeaponLocation`/`ArmorLocation`/`DefensiveHorseLocation`/`OffensiveHorseLocation`/`TreasureLocation`），不创建实例。
- `qml/Dashboard.qml`/`qml/Photo.qml`：5 个装备槽索引从魔数 `[0..4]` 改为 `EquipCard.WeaponLocation` 等枚举名。
- **桥接层简化**：`roomscene.cpp` `move_cards_got`/`lost` lambda 去掉 `equipLocations` 解析（dynamic_cast EquipCard）与 `#include "standard.h"`；QML `onNotifyMoveCardsGot/Lost` 改读 `Sanguosha.getEngineCard(cardId).getRealCard().location` 得槽位（getEngineCard 返回 WrappedCard，须 getRealCard 拿真实 EquipCard；详见"Qt6 moc / QML 约束"getRealCard 条）。槽位信息不再经桥接层传，QML 直读 Card Q_PROPERTY。

### 2026-07-23：A-TablePile 同步实现
- **新建 `qml/TablePile.qml`**：Item 根（`anchors.centerIn: parent`，width `parent.width*0.45`/height 256）内含 CardContainer（id `pile`，`rootScene` 透传）。`addCard(cardId)`/`removeCard(cardId)` 走 `pile.createItem`/`removeItem` + `lay(Qt.AlignHCenter,1,0,true,true)`。延迟清除对标旧 `TablePile.cpp`：`clearTimestamps`（{}，cardId->timestamp，新牌 `Number.MAX_VALUE`）+ `currentTime`（Timer 每秒 +1，`running: pile.cardItems.length>0`）；`_markOverflowClearance` 超量（`length - max(numVisible, numAdded+1)`）时给最老未标记牌设时间戳；`_checkClearance` 倒序遍历，`currentTime - ts > sClearanceDelayBuckets(3)` 的牌从 cardItems splice + `destroy()` + 删时间戳，有移除则重排。`numCardsVisible = floor(width/183)+1`（对标旧 `setSize`）。TablePile 统一处理 `PlaceTable`/`PlaceJudge`/`DiscardPile`，不区分；`PlaceDelayedTrick`（判定区容器）/`PlaceSpecial`（私人牌堆）不在本次范围。
- **`qml/CardContainer.qml`**：`lay` 加 `Qt.AlignHCenter` 单行居中分支（`x = (width - (step*(length-1)+cardWidth))/2`，负则 0），TablePile 用；手牌区 `AlignLeft` 不受影响。
- **`qml/RoomScene.qml`**：加 `TablePile { id: tablePile; rootScene: roomScene }` 实例（SkillDock 后）；`onNotifyMoveCardsGot` 加 `toPlace === PlaceTable||PlaceJudge||DiscardPile` 分支调 `tablePile.addCard`，`onNotifyMoveCardsLost` 加 `fromPlace` 同三者分支调 `tablePile.removeCard`；注释更新（"Judge/pile/table deferred" → "Table pile ... routed to tablePile; Judge area(PlaceDelayedTrick)/private piles(PlaceSpecial) deferred"）。
- **`.pro`** OTHER_FILES 加 `qml/TablePile.qml`。
- TODO：TablePile 的 `showJudgeResult`（判定结果高亮，旧 `TablePile.cpp:104`）未移植；牌区进出场动画（手牌/装备/TablePile 当前简化"新建+销毁"）待"CardItem 移动动画"任务统一做原实例移动 + 坐标映射起点 + 飞行。

### 2026-07-24：TablePile 内部 place 互转跳过
- `qml/RoomScene.qml`：加 `isTablePilePlace(place)` 辅助函数（`PlaceTable`/`PlaceJudge`/`DiscardPile`）；`onNotifyMoveCardsGot` TablePile 分支加 `&& !isTablePilePlace(m.fromPlace)`，`onNotifyMoveCardsLost` 加 `&& !isTablePilePlace(m.toPlace)`。`PlaceTable`/`PlaceJudge`/`DiscardPile` 三者间互转的 move，got/lost 都跳过（牌原位保留，不改顺序、不重启动画）；仅从外部进入或去外部才 add/remove。

### 2026-07-24：z 属性约定放宽（静态禁用、动态放宽）
- UI 约定"z 属性"条目更新：旧版约定全面禁用 z（静态/动态都不用，靠声明顺序）。本次因动态创建对象（`CardContainer.lay` 的 `cardItems[i].z = i`、动态弹窗置顶等）运行时创建顺序无法在文件中靠声明顺序控制，放宽为：静态创建对象（QML 直接声明）禁用 z；动态创建对象（`createObject`/Loader）可用 z 但严格限制范围（仅声明顺序无法解决的动态堆叠场景）。静态禁用沿用旧版理由（旧 `src/uibackup/` 滥用 z 到万级）。

### 2026-07-24：A-判定区 + 私人牌堆实现
- **`src/core/player.h`**：`getPile(name)`/`getPileNames()` 加 `Q_INVOKABLE`（QML 直调，返回 QVariantList/QStringList）。
- **新建 `qml/JudgeArea.qml`**：判定区（`PlaceDelayedTrick`），每张牌一个图标（不显示 CardItem），对标旧 `PlayerCardContainer::addDelayedTricks`。`property var cardIds: []` + `addDelayedTrick(cardId)`（`concat` 赋值触发 Repeater 更新）/`removeDelayedTrick(cardId)`（`filter` 赋值）。图标 `image/icon/<objectName>.png`（`defaultSkin.image.json: judgeCardIcon-default`，objectName 从 `Sanguosha.getEngineCard(cardId).objectName` 取）。Row + Repeater 横向排列。
- **新建 `qml/PrivatePileArea.qml`**：私人牌堆（`PlaceSpecial`），旧版按钮按住显示牌框改为**下拉菜单**。`property var player` + `pileNames`（`player.getPileNames()`）+ `activePile`（当前展开的 pile）。`Component.onCompleted`/`onPlayerChanged`/`Connections onPileChanged` 调 `refresh()` 重建 pileNames。Repeater of 按钮（`Sanguosha.translate(name)` + `(数量)`），点击 `togglePile(name)`。下拉菜单（`Rectangle` popup）显示 `player.getPile(activePile)` 的 CardItem Repeater（cardId=-1 显示牌背），点击 popup 背景关闭。pile 牌变化由 `pile_changed` 驱动（不走 move 分发）。
- **`qml/Dashboard.qml`**/`qml/Photo.qml`：加 `property alias judgeArea` + `JudgeArea` 实例（Dashboard + Photo 各一个，Photo 的 `visible: !selfPhoto`）+ `PrivatePileArea` 实例（每个 Photo 一个，self/非self 都显示，Dashboard 不另建）。
- **`qml/RoomScene.qml`**：`onNotifyMoveCardsGot` 加 `toPlace === PlaceDelayedTrick` 分支（按 `toPlayer` 分发 `dashboard.judgeArea`/`photo.judgeArea.addDelayedTrick`），`onNotifyMoveCardsLost` 加 `fromPlace === PlaceDelayedTrick` 分支（`removeDelayedTrick`）。`PlaceSpecial` 不加 move 分发（PrivatePileArea 内部 `pile_changed` 驱动）。注释更新。
- **`.pro`** OTHER_FILES 加 `qml/JudgeArea.qml`/`qml/PrivatePileArea.qml`。
- A 子任务（牌区同步）全部完成：手牌✅/装备✅/TablePile✅/判定✅/私人牌堆✅。
- TODO：判定区图标 tooltip（卡名+花色+点数，旧版 `addDelayedTricks` 设 `setToolTip`）、PrivatePileArea 下拉菜单的 pile 牌点击交互（查看详情/选中）、treasure pile（`wooden_ox`）排最前（旧版 `updatePile` 特殊处理）留待后续；layout 位置待统一调。

## 7. 下一步
1. **CardItem 卡牌选择全链路**（已拆分为 A-G 子任务，见"待做"小节）：
   - 执行顺序：A（牌区同步：手牌✅/装备✅/TablePile✅/判定✅/私人牌堆✅）→ B（CardItem 选中态）→ C（Dashboard pending + PromptBox）→ D（桥接选卡回传）→ E（OK/Cancel/Discard 按钮回传）→ F（目标选择）。
   - 第一阶段最小闭环：A+B+C+D+E（先只支持 `targetFixed` 卡：桃自用、无懈可击等），F 补目标选择（杀/闪/桃救人），G 与"Dashboard 技能按钮"任务合并。
2. 选项/触发顺序弹窗（ChooseOptionsBox/ChooseTriggerOrderBox）。
3. 玩家牌展示/桌面牌堆、聊天日志。
4. Dashboard 技能按钮 + 装备区（含 G 子任务：ViewAsSkill 组牌；旧版实现参考见"装备区与技能按钮（旧版实现参考）"小节，子任务见"待做"小节）、RoomScene 测试桩清理、src/uibackup 删除。
5. QML 重构完成后：Client 去单例化（见 `client.h` TODO；桥接 `selfHelper`/`clientHelper` 已预留注入点，自包含 dialog 需一并改造）。
