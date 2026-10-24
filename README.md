# Thirsty 口渴模组

基于 LeviLamina **26.51.5**，提供 Windows x64 客户端和服务端两个版本。

## 安装

- **单人存档**：只安装客户端版本，把 `bin/client/thirsty` 整个文件夹放入客户端 LeviLamina 的 `mods` 目录。内置服务端负责计算，本地直接显示口渴条，不需要另外启动 BDS。
- **多人服务器**：将 `bin/server/thirsty` 放入服务端的 `plugins` 目录，玩家客户端同时安装客户端版本。口渴值由服务器计算并同步。
- 客户端和服务端均须使用匹配的 LeviLamina 与游戏版本。客户端连接未安装本模组的远端服务器时不显示口渴条，也不会自行计算远端玩家的口渴值。

**物品包随模组自动加载**：

- 安装或更新时复制对应版本的完整 `thirsty` 文件夹，包含 DLL、`behavior_packs` 和 `resource_packs`，然后重启游戏或服务器。
- 模组在加载阶段注册自带的两个包，游戏将它们作为系统包自动启用。单人新建、已有存档以及 BDS 使用同一套流程，无需手动导入 `.mcaddon`、勾选世界附加包或编辑世界包列表。
- 包文件直接从模组目录加载，随完整模组文件夹一起更新；服务端通过游戏原有流程向客户端提供资源包。

客户端在原版饥饿条上方绘制 **10 个水滴**，每个水滴表示 2 点，支持半格；剩余 4 点及以下变为橙色。口渴条本身无需资源包，随原版饥饿 HUD 显示和隐藏，位置可在配置中调整。

## 水壶、净水和脏水

| 物品或操作 | 行为 |
| --- | --- |
| 普通水瓶 | 视为脏水；喝完恢复 6 点，默认有 30% 概率获得 20 秒反胃 I |
| 纯净水瓶 | 普通水瓶在熔炉中加热获得；恢复 6 点，无脏水负面效果，喝完返还玻璃瓶 |
| 空水壶 | 使用 5 个铁锭合成，容量 4 口，每口恢复 6 点 |
| 水面取水 | 手持空水壶对准水面右键，装满 4 口脏水，不消耗水源；流动水也可取水 |
| 水桶补水 | 空壶与水桶在合成栏中合成，得到 4 口脏水，返还空桶 |
| 水瓶补水 | 空壶或未满的同水质水壶，加一瓶水补充 1 口，返还玻璃瓶；纯净水瓶补入净水，普通水瓶补入脏水 |
| 水壶净化 | 装有脏水的水壶放进熔炉，转成相同剩余口数的净水壶 |
| 喝完水壶 | 保留空水壶，可再次补水 |

空水壶配方（`I` 表示铁锭，空格表示空槽）：

```text
I I
I I
 I
```

水壶不堆叠，名称显示水质和剩余口数，图标中的水位也会变化。水质和口数随物品保存，放入箱子、丢弃、转交及退出重进均使用对应物品状态。净水和脏水不能混装，可先喝空或净化后再补入相同水质的水。

水壶与纯净水瓶使用 32×32 像素材质；水壶带皮革包边、金属瓶盖和水位窗，蓝色表示净水，黄绿色表示脏水。纹理绘制源文件为 `tools/draw_item_textures.py`（需要 Pillow）。

熔炉净化水瓶和使用水瓶补水的配方仅匹配普通水瓶（`minecraft:potion`，数据值 `0`），不消耗其他药水或喷溅水瓶。饮用只恢复口渴值，不增加饥饿值；喝到一半取消不会恢复或触发反胃。创造、旁观不获得脏水效果。

创造物品栏包含空壶、满脏水壶、满净水壶和纯净水瓶。也可以使用：

```mcfunction
/give @s thirsty:flask
/give @s thirsty:purified_water_bottle
/give @s thirsty:dirty_flask_4
/give @s thirsty:clean_flask_4
```

`dirty_flask_1`～`dirty_flask_4`、`clean_flask_1`～`clean_flask_4` 分别保存 1～4 口的状态，中间状态不占用创造物品栏。

## 默认规则

| 项目 | 行为 |
| --- | --- |
| 最大口渴值 | 20 |
| 生效模式 | 生存、冒险；创造、旁观不消耗且不显示 |
| 普通消耗 | 每 60 秒减少 1 点 |
| 疾跑消耗 | 普通状态的 2 倍 |
| 恢复方式 | 喝完普通水瓶、纯净水瓶或一口水壶恢复 6 点，上限 20；中途取消、其他药水、喷溅水瓶不恢复 |
| 脏水效果 | 普通水瓶或脏水壶每喝完一次，有 30% 概率反胃 20 秒；净水不触发 |
| 口渴耗尽 | 每 4 秒造成 1 点饥饿类型伤害（半颗心），可关闭 |
| 重生 | 恢复为 20 点 |
| 存档 | 数值和消耗进度随当前世界的玩家 NBT 保存，退出重进继续计算 |

计时使用游戏 tick（20 tick = 1 秒），真正暂停世界时停止计算；游戏卡顿时随游戏时间变慢。

## 配置

首次加载后在模组的 `config/config.json` 生成配置，修改后重新启动游戏或服务器。

```json
{
    "version": 2,
    "secondsPerPoint": 60,
    "sprintMultiplier": 2,
    "waterBottleRestore": 6,
    "dirtyWaterDebuffChance": 30,
    "dirtyWaterDebuffSeconds": 20,
    "damageWhenEmpty": true,
    "damageIntervalSeconds": 4,
    "damageAmount": 1.0,
    "hudOffsetX": 0.0,
    "hudOffsetY": -20.0
}
```

`hudOffsetX`、`hudOffsetY` 用于微调水滴条相对饥饿条的位置，以游戏 UI 单位计量；负 Y 向上。单人使用客户端配置；联机玩法参数使用服务器配置，HUD 位置使用客户端配置。

`waterBottleRestore` 对每瓶水及水壶每口水生效。`dirtyWaterDebuffChance` 是 0～100 的百分比，设为 0 关闭脏水效果；`dirtyWaterDebuffSeconds` 是反胃持续秒数。旧版配置加载时会补入这两个字段并保留已有参数。

## 编译

需要 xmake、Visual Studio C++ 工具链和 clang-cl。在项目目录执行：

```powershell
.\build.ps1
```

分别输出到 `bin/server/thirsty` 与 `bin/client/thirsty`，每个发布目录均包含 DLL 和自动加载所需的两个包，两端 DLL 不可混用。单独运行 `xmake` 时，`bin/thirsty` 同样包含两个包。

也可以只编译指定版本（单次打包输出为 `bin/thirsty`）：

```powershell
xmake f -y -p windows -a x64 -m release --target_type=server
xmake -y

xmake f -y -p windows -a x64 -m release --target_type=client
xmake -y
```

`src/mod/ThirstSystem.cpp` 为两端共用的服务端逻辑，客户端在单人/本地联机世界运行这部分；`src-client/ThirstHud.cpp` 负责客户端绘制；`ThirstPacket` 负责同步。
