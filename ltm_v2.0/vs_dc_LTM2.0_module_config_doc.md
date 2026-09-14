# vs_dc Feature 模块配置文档 —— LTM2.0

> 对应 `vs_dc_Feature_module_config_doc_template.docx` 第二节模板。
> 读者：软件驱动工程师。只讲**软件怎么把参数配到硬件、数据怎么流动、代码在哪**，
> 不讲硬件电路原理。
> 引用代码一律只给**文件名 + 函数名**，不给行号（行号会随代码变动）。

**Feature 名称：LTM2.0　DRM property：`"LTM"`　作用对象：CRTC（Display / postprocess 域）**

---

## 快速上手（先看这 7 条）

1. **LTM2.0 = 一套硬件 + 三种模式**：LTM（局部对比度）、SRE（阳光可读）、FOSS（保真缩放），
   靠 `drm_vs_ltm.ltm2_mode` 切换。
2. **算法在用户态，不在驱动里**。驱动只做"参数搬运 + 寄存器映射"，一行算法都没有。
3. **所有参数走一个 blob property `"LTM"`**（`struct drm_vs_ltm`，约 50KB），
   CRTC 上 atomic set 一次性下发。
4. **这是个跨帧反馈闭环**：算法输入 = case 参数 + **上一帧**硬件回读值。
   所以 **case 必须是多帧序列**，单帧没意义。
5. **能力位两级**：`info->ltm2_mask`（芯片，按位声明支持哪些 mode，dc9400 上是 `0x7`）
   + `display_info->ltm`（该 display 是否有 LTM）。
6. **跑一个 case**：
   `python3 script/dtest.py -d auto_cases/0x2000003d -o /tmp/dump --soft -f '*postpq*ltm*'`
7. **排查顺序**：`state->dirty` → `ltm_check()` 有没有拒绝 → `ltm2_mask` 分流对不对
   → shadow / non-shadow 写入时机 → 回读 `read_done`。详见 2.9。

---

## 2.0 接口层归类

接口层：☐ L1 标准DRM　☑ L2 自定义私有属性　☑ L3 proto框架

LTM2.0 **跨两层**，下行配置和上行回读走的不是一套机制：

| 方向 | 接口层 | Property | 说明 |
|---|---|---|---|
| 下行 配置硬件 | **L3 proto 框架** | `"LTM"`（BLOB） | `VS_DC_BLOB_PROPERTY_PROTO(ltm_proto, ...)` 注册到 `display->states`，由 `vs_dc_create_drm_properties()` 自动生成，走 `dirty` flag |
| 上行 回读统计量 | **L2 私有属性** | `"LTM_HW_READOUT"`（IMMUTABLE BLOB） | `vs_crtc_create()` 里手写 `drm_property_create()`；驱动在中断 worker 里**原地改 blob->data**，用户态只读 |
| 下行 LTM1.0 遗留 | L3 | `"LTM_AF_FILTER"` | 仍会注册，但 LTM2.0 上 check 直接返回 `-EOPNOTSUPP` |
| 上行 LTM1.0 遗留 | L2 | `"LTM_LUMA_AVE_GET"`<br>`"LTM_HIST_CD_GET"`<br>`"LTM_LOCAL_HIST_GET"` | 仅 `display_info->id == 0` 时创建，LTM2.0 流程不用 |

判断依据：`"LTM"` 挂在 `hw_display->states.items[]` 里（`vs_dc_property_register_state()`）→ L3；
`"LTM_HW_READOUT"` 是手写 `drm_property_create()` + `drm_object_attach_property()` → L2。

---

## 2.1 概述

### 2.1.1 一句话 + 三种模式

**一句话**：把画面切成最多 8×8 个 block，用户态算法根据上一帧硬件回读的直方图/统计量
算出本帧每个 block 的映射曲线，通过 `"LTM"` blob 下发，驱动拆成几十个寄存器写下去。

| 模式 | 全称 | `ltm2_mode` | 做什么 |
|---|---|---|---|
| **LTM** | Local Tone Mapping<br>局部色调映射 | `VS_LTM_LTM` (0) | 分区调整明暗与对比度，暗部出细节、亮部保层次 |
| **SRE** | Sunlight Readability Enhancement<br>阳光可读增强 | `VS_LTM_SRE` (1) | 按环境光自适应调整像素值，解决户外强光下看不清 |
| **FOSS** | Fidelity-Oriented Signal Scaling<br>高保真信号缩放 | `VS_LTM_FOSS` (2) | 缩放场景下保细节，配合 ROI 文本判定避免文字劣化 |

三种模式**共享同一套分块统计、同一套 affine 映射引擎、同一个 property**，
区别只在：调哪个算法入口、需要哪些统计量、使能哪些子模块（见 2.2.4）。

### 2.1.2 设计目的

**总目标：对比度更丰富、强光下更清晰，同时功耗更低。**

| 目的 | 怎么达到 | 模式 |
|---|---|---|
| 提升局部对比度 | 分块映射曲线，暗部提亮不牺牲亮部 | LTM |
| 提升户外可视性 | 按环境光调像素值，**而不是拉高背光** | SRE |
| **降低功耗** | 上一条的延伸 —— 降低对高亮度背光的依赖 | SRE |
| 保持缩放保真 | ROI / 文本区单独判定 | FOSS |

> 功耗是 LTM2.0 相对 1.0 的主要增量价值：提高背光会显著增加功耗，
> SRE 把"看得清"从背光域挪到了图像处理域。

对驱动的设计约束：算法留在用户态；参数用**一个 blob** 一次下发（保证跨帧一致性）；
提供稳定的**回读通道**给算法用。

### 2.1.3 代码位置

驱动侧（`drm-driver/`）：

| 关注点 | 文件 | 函数 |
|---|---|---|
| proto 定义 / 注册 | [verisilicon/9x00/postprocess/vs_dc_ltm.c](drm-driver/verisilicon/9x00/postprocess/vs_dc_ltm.c) | `ltm_proto`、`vs_dc_register_ltm_states()` |
| 注册调用点 | [verisilicon/9x00/postprocess/vs_dc_postprocess.c](drm-driver/verisilicon/9x00/postprocess/vs_dc_postprocess.c) | `vs_dc_register_postprocess_states()` |
| 参数校验 | vs_dc_ltm.c | `ltm_check()`、`ltm_af_filter_check()` |
| 版本分流 | vs_dc_ltm.c | `ltm_config_hw()` → `ltm2_hw_config()` / `ltm_hw_config()` |
| **LTM2.0 各子模块配寄存器** | vs_dc_ltm.c | `ltm2_*_config_hw()` 共 13 个 |
| check 分发 | [verisilicon/vs_dc_drm_property.c](drm-driver/verisilicon/vs_dc_drm_property.c) | `vs_dc_check_drm_property()` |
| check 调用点 | [verisilicon/9x00/vs_dc_post.c](drm-driver/verisilicon/9x00/vs_dc_post.c) | `vs_dc_check_display()` |
| blob 落地 + 置 dirty | [verisilicon/vs_dc_property.c](drm-driver/verisilicon/vs_dc_property.c) | `vs_dc_blob_property_update()` |
| commit 遍历 states | [verisilicon/9x00/vs_dc_hw.c](drm-driver/verisilicon/9x00/vs_dc_hw.c) | `display_commit()` |
| `LTM_HW_READOUT` 创建 | [verisilicon/vs_crtc.c](drm-driver/verisilicon/vs_crtc.c) | `vs_crtc_create()` |
| 清 `read_done` | vs_dc_hw.c | `ltm_set_read_done()`（`display_commit()` 里调） |
| 中断触发回读 | [verisilicon/9x00/vs_dc.c](drm-driver/verisilicon/9x00/vs_dc.c) | `dc_isr()` → `schedule_work(&crtc->ltm_isr_work)` |
| 回读 worker | vs_dc.c | `vs_dc_handle_ltm_isr_work()` |
| 实际读寄存器 | vs_dc_hw.c | `dc_hw_get_display_ltm2_data()` |
| non-shadow 寄存器 flush | vs_dc_post.c | `vs_dc_commit_non_shadow_reg_cache()` |
| UAPI 结构体 | [include/uapi/drm/vs_drm_api.h](drm-driver/include/uapi/drm/vs_drm_api.h) | `struct drm_vs_ltm`、`struct drm_vs_ltm_hw_readout`、`enum drm_vs_ltm_mode` |
| 能力位 | verisilicon/9x00/info/vs_dc_info_9400_\<CID\>.c | `.ltm = 1`、`.ltm2_mask = 0x7` |
| 编译宏 | verisilicon/9x00/func/vs_dc_option_\<CID\>.h | `CONFIG_VERISILICON_LTM` |

测试侧（`drm-test/`）：

| 关注点 | 文件 | 函数 |
|---|---|---|
| property 表登记 | [src/prop_mapper.c](drm-test/src/prop_mapper.c) | `PROP_BLOB_ITEM("LTM", map_drm_ltm)` |
| **LTM2.0 主路径** | [src/map_funcs/prop_map_funcs.c](drm-test/src/map_funcs/prop_map_funcs.c) | `map_drm_ltm()` → `_map_drm_ltm()` |
| vcmd 路径（仅 LTM1.0） | src/map_funcs/prop_map_funcs.c | `_map_drm_ltm_vcmd()`、`update_ltm_slope_bias()` |
| JSON 参数解析 | [src/map_funcs/json_prasing.c](drm-test/src/map_funcs/json_prasing.c) | `get_ltm_params_from_json()` |
| 用户参数结构 | [include/map_funcs/json_prasing.h](drm-test/include/map_funcs/json_prasing.h) | `struct ltm_algo_params` |
| algo .so 接口 / 加载 | [include/dpu_algorithm.h](drm-test/include/dpu_algorithm.h)、[src/dpu_algorithm.c](drm-test/src/dpu_algorithm.c) | `struct pq_helper_funcs`、`dpu_algorithm_lib_init()` |
| 回读取值 | [src/drmtest_helper.c](drm-test/src/drmtest_helper.c) | `dtest_get_ltm_hw_readout_result()` |
| 每帧调用点 | [tools/drmtest.c](drm-test/tools/drmtest.c) | 主循环 |
| case 生成器 | [script/dtest/dc9400/crtc/feature/postpq/ltm2.py](drm-test/script/dtest/dc9400/crtc/feature/postpq/ltm2.py) | — |

---

## 2.2 功能原理

### 2.2.1 处理流程（四步）

1. **分块**：按 `grid_size.width × height` 切分，LTM2.0 最大 **8×8 = 64 block**（1.0 是 12×12）。
2. **统计**：硬件对每 block 统计直方图（默认 64 bin）和 avg_luma / sd_ratio / wopr / sum_roi
   等标量。直方图走 **WDMA 写内存**，标量走 **APB 寄存器**回读。
3. **算曲线**：用户态算法库用"上一帧统计量 + 用户参数"算出每 block 的仿射映射系数
   （`ltm_af_lut`，64×17 = 1088 个 U11）和各子模块开关/阈值。
4. **重建**：硬件按 affine LUT 逐像素做局部映射；非 ROI 区按 `non_roi_mapping_way`
   取最近 block 曲线；再经 tone adjust / color protect / dither 输出。

### 2.2.2 核心：跨帧反馈闭环

这是 LTM2.0 与其它 postprocess feature 最大的不同：

```
  frame N-1                        frame N
  ┌──────────┐                     ┌────────────┐
  │  HW 统计  │── hist (WDMA) ────▶ │ 用户态读取  │
  │  + WDMA  │── avg  (APB)  ────▶ │  上帧结果   │
  └──────────┘                     │     ↓      │
                                   │  algo .so  │
                                   │     ↓      │
                                   │ drm_vs_ltm │
                                   │     ↓      │
                                   │ "LTM" blob │──▶ 驱动 config_hw ──▶ 寄存器
                                   └────────────┘
```

- 第 0 帧无历史数据，统计量全 0，算法走默认值路径。
- 帧间平滑（IIR）由 `temporal_smooth` / `Sensity` / `IIR_Wgt_max` 控制，
  算法内部用 `cdf_curve_last[][]` 维护跨帧状态。
- **所以 case 必须是多帧序列** —— `ltm2.py` 里的 case 都是 3 帧或 9 帧。

### 2.2.3 在 pipeline 中的位置

LTM 属于 **postprocess（per-display / panel 域）**，不是 per-layer 的 preprocess。
按 `vs_dc_register_postprocess_states()` 的注册顺序，位于 blur / pvric 之后、
panel CCM / panel degamma / scale / histogram 之前。
总开关是 `DCREG_SH_PANEL{0,1}_CONFIG.LTM`，与其它 panel 级模块并列。

### 2.2.4 三种模式的差异（软件视角）

| | LTM | SRE | FOSS |
|---|:-:|:-:|:-:|
| 算法入口 | `dpu_algo_api_ltm` | `dpu_algo_api_ltm_sre` | `dpu_algo_api_ltm_foss` |
| 需要直方图（WDMA） | ✅ | ❌ | ✅ |
| 需要 `avg_luma` / `avg_sd` | ❌ | ✅ | ✅ |
| 需要 `avg_wopr` | ❌ | ✅ | ❌ |
| 需要 `sum_txt`（ROI） | ❌ | ❌ | ✅ |
| 走 APB 回读通路 | ❌ | ✅ | ✅ |
| 主强度参数 | `Strength` | `luma_luma_intensity` | `Strength` + `foss_strength` |

代码对应：`_map_drm_ltm()` 里 `if (p.ltm2_mode != VS_LTM_SRE)` 决定是否建/读直方图 BO；
`dc_hw_get_display_ltm2_data()` 里按 mode 决定读哪些标量寄存器
（`ltm2_mode == VS_LTM_LTM` 时直接 `return`，一个都不读）。

---

## 2.3 接口映射

### 2.3.1 L3：`"LTM"` 配置通道

在 `vs_dc_ltm.c` 里定义：

```c
VS_DC_BLOB_PROPERTY_PROTO(ltm_proto, "LTM", struct drm_vs_ltm,
                          ltm_check,     /* check    */
                          NULL,          /* update   —— 无 */
                          ltm_config_hw);
```

用户态 `drmModeCreatePropertyBlob()` + atomic set 一次性下发整个结构体。

**能力开关（四级）**

| 层级 | 开关 | 位置 | 说明 |
|---|---|---|---|
| 编译宏 | `CONFIG_VERISILICON_LTM` | func/vs_dc_option_\<CID\>.h | 整个 `vs_dc_ltm.c` 被 `#ifdef` 包住 |
| 编译宏 | `DCREG_SH_PANEL0_LTM2_CONFIG_Address` | register set | **寄存器存在性**开关；老 register set 下所有 `ltm2_*` 函数不参与编译，`ltm2_hw_config()` 退化为空 |
| 芯片能力位 | `info->ltm2_mask` | info/vs_dc_info_9400_\<CID\>.c | `0` = LTM1.0；非 0 = LTM2.0，**按位**表示支持哪些 mode（`0x7` = 三种都支持） |
| Display 能力位 | `display_info->ltm` | 同上 | 决定该 display 是否注册 `ltm_proto` |

已知 `ltm2_mask = 0x7` 的 CID：`0x20000034` / `0x20000039` / `0x2000003d`（均 dc9400）。

**没有 `update` 回调**，走 `vs_dc_blob_property_update()` 默认分支：

```c
if (!new_data) {                  /* blob = 0 → 关闭 */
        state->dirty  = state->enable;    /* 原来开着才需要写寄存器 */
        state->enable = false;
} else {                          /* 下发 blob → 开启 */
        state->dirty  = true;
        memcpy(state->data, new_data, state->proto->type_size);
        state->enable = true;
}
```

**为什么不需要 update**：LTM2.0 的"动态性"全在用户态算法里（算法据上一帧回读值决定各
子模块开关），驱动拿到的已是最终结果，不需要在内核里按 fb 格式之类再改 `enable`。
副作用：驱动**不缓存**上一帧参数做差分，只要下发新 blob 就整套重写。

### 2.3.2 L2：`"LTM_HW_READOUT"` 回读通道

在 `vs_crtc_create()` 里创建，条件是 **`display_info->ltm && info->ltm2_mask`**
（注意与 `LTM_LUMA_AVE_GET` 那组的条件不同，后者要求 `display_info->id == 0`）：

```c
crtc->ltm_hw_readout = drm_property_create(drm_dev,
        DRM_MODE_PROP_IMMUTABLE | DRM_MODE_PROP_BLOB, "LTM_HW_READOUT", 0);
blob = drm_property_create_blob(drm_dev, sizeof(struct drm_vs_ltm_hw_readout), NULL);
drm_object_attach_property(&crtc->base.base, crtc->ltm_hw_readout, blob->base.id);
```

要点：

- **IMMUTABLE** —— 用户态不能 set，没有 set/get 自定义分支；
- blob 创建时就分配好，驱动**原地改 `blob->data`**，不进 `vs_crtc_state`，
  不参与 state 复制，无引用计数处理
  （而 `ltm_luma_get` / `ltm_cd_get` / `ltm_hist_get` 三个遗留 blob 需要
  `drm_property_blob_get/put()`）；
- 取值方式：`drm_object_property_get_default_value()` 拿 blob_id → `drm_property_lookup_blob()`
  （kernel < 5.19 走手动遍历 `mode_obj->properties[]`）；
- **`read_done` 握手**：`display_commit()` 下发本帧配置时 `ltm_set_read_done()` 清 0，
  中断 worker 读完寄存器后置 1，用户态轮询（10 × 1ms）。

---

## 2.4 参数校验规则

入口 `ltm_check()`，由 `vs_dc_check_drm_property()` 在 atomic_check 阶段分发
（调用点 `vs_dc_check_display()`）。**只有 `state->is_changed` 为真才 check，
且 `blob == NULL`（关闭）时直接跳过。**

| 参数 | 约束 | 失败返回 |
|---|---|---|
| `ltm2_mode` | `ltm2_mask == 0` 时只能是 `VS_LTM_LTM` | `-EOPNOTSUPP` |
| `grid_size.width/height` | **LTM2.0：`[1, 8]`**；LTM1.0：`[1, 12]` | `-EOPNOTSUPP` |
| `luma_adj.entry_cnt` | 必须 = `VS_MAX_1D_LUT_ENTRY_CNT` (129)，仅 `luma_adj.enable` 时校验 | `-EINVAL` |
| `tone_adj.entry_cnt` | 必须 = `VS_LTM_TONE_ADJ_COEF_NUM` (129)，仅 `tone_adj.enable` 时校验 | `-EINVAL` |
| `"LTM_AF_FILTER"` 整个 property | LTM2.0 下不允许配置（`ltm_af_filter_check()`） | `-EOPNOTSUPP` |

依赖的能力字段：`hw->info->ltm2_mask`、`display_info->ltm`、`display_info->gtm`。

**驱动不校验什么**（排查花屏/参数不生效时的重点方向）：

- **不依赖 crtc_state / fb** —— `ltm_check()` 的 `obj_state` 参数根本没用，
  **ROI / DS output 尺寸与实际 display mode 的一致性完全不校验**；
- `ltm_af_luts.entry_cnt` 不校验（`ltm2_af_lut_config_hw()` 按硬编码 64×17 写）；
- 各子模块 `enable` 的合法组合不校验（由算法库保证，见附录 C 的使能矩阵）；
- 用户态参数范围（`Strength` / `Sensity` 等）不在驱动校验，见附录 A。

---

## 2.5 提交路径与时序

### 2.5.1 靠 dirty flag，不是每次 commit 都写

```
用户态 atomic set "LTM" blob
  → vs_crtc_atomic_set_property()   → vs_dc_set_drm_property()     记录 blob，置 is_changed
  ↓ atomic_check
  → vs_dc_check_drm_property()      → ltm_check()
  ↓ atomic_commit
  → vs_dc_blob_property_update()    memcpy 到 state->data，dirty = true
  → display_commit()                if (!state->dirty) continue;        ← 关键
      → vs_dc_property_config_hw()  → ltm_config_hw()，成功后 dirty = false
```

但 LTM2.0 **每帧都下发新算法结果 → 每帧 dirty 都是 true**，
所以整套寄存器每帧重写。这是它寄存器写入量大、对 APB / command buffer 带宽敏感的原因。

### 2.5.2 子模块配置顺序

`ltm2_hw_config()` 固定顺序：

1. **`ltm2_enable_config_hw()` 必须第一个** —— 它同时做三件事：
   把 `ltm->ltm2_mode` 存进 `display->ltm2_mode`（后续中断/回读逻辑依赖它）、
   写 `PANEL_CONFIG.LTM` 总开关、写 `LTM2_CONFIG` 的 `COLOR_PROTECT` / `AD_TXT_JUD` 两位。
2. `enable == false` 时**后面全部跳过**（见 2.5.5）。
3. `enable == true` 时依次：`luma` → `freq_decomp` → `grid` → `af_slice` → `tone_adj`
   → `dither` → `hist_cd_set` → `local_hist_set` → `ds` → `local_hist_get`
   → `af_lut` → `sre_wopr` → `sre_sd`。

> **`LTM2_CONFIG` 是读改写 + 软件影子**：每个子模块函数从 `hw->panel_mask[i].ltm2_config`
> 读出当前值、改自己那一位、写寄存器、再更新影子。
> 所以**调用顺序不影响最终值，但哪个函数忘了更新影子，后面的函数就会把它的位清掉** ——
> 排查"某个使能位莫名为 0"先看这里。影子初值在 `dc_hw_init_mask()` 里设为 ResetValue。

### 2.5.3 shadow / non-shadow（最容易出时序问题的地方）

LTM2.0 同时用了两种写法：

| 写函数 | 目标 | 生效时机 | LTM2.0 用它写什么 |
|---|---|---|---|
| `dc_write()` | `DCREG_SH_PANEL*`（shadow） | 下一次 frame start 由硬件从 shadow 载入 | `LTM2_CONFIG`、`GRID_CONFIG`、`GRID_SCALE`、`BLOCK_RATIO_*`、`TONE_ADJUST_LUT`、`CD_MIN_WGT`、`HIST_GRID_SCALE`、`DS_*`、`ROI*`、`LOCAL_HIST_WB_*ADDRESS` |
| `dc_write_non()` | `DCREG_PANEL*`（non-shadow） | **先进软件缓存**，由 `vs_dc_commit_non_shadow_reg_cache()` **等 frame done 中断后**才真正 `writel` | `RGBY_COEF`、`GRAY_LIGHT_WGT`、`FREQ_DECOMP_COEF_A/B`、`DITHER_TABLE`、`CD_COEF/THR/SLOPE`、`HIST_OVERLAP_RATIO`、**`AFFINE_LUT`**、`WOPR_THR`、`SD_BLEND_THR_A/B` |

（`dc_write_immediate()` LTM2.0 不用，只有 LTM1.0 的 `ltm_af_filter_config_hw()` /
`gtm_config_hw()` 用它写 `LTM_CONFIG.REG_SWITCH`。）

**实践含义：**

- **最核心的 `AFFINE_LUT`（1088 项映射曲线）走 non-shadow** —— 它不是 commit 时写进硬件的，
  而是**等上一帧 frame done 之后**才写。"映射曲线晚一帧生效"类问题从这里查。
- non-shadow 缓存有容量上限 `VS_MAX_NON_SHADOW_ENTRY_CNT`。LTM2.0 每帧往里塞
  1088/2 = 544 个 `AFFINE_LUT` 写 + 十几个其它寄存器，**多 pipe 同开 LTM 时注意是否溢出**。
- `AFFINE_LUT` / `TONE_ADJUST_LUT` / `*_DATA` 都是**同地址反复读写的数据口**
  （硬件内部地址自增），不是地址递增的寄存器数组 —— **写入/读取次数必须精确**，
  多一次少一次都会错位。

### 2.5.4 回读时序

```
frame N commit   display_commit()
                   ├─ ltm_config_hw() 写寄存器
                   └─ if (ltm2_mode == SRE || FOSS) ltm_set_read_done() → read_done = false
                 ↓ 硬件跑一帧
frame N done     dc_isr()
                   if (ltm2_mask && display[i].ltm2_mode != VS_LTM_LTM)
                       schedule_work(&crtc[i]->ltm_isr_work)
                 ↓ workqueue（进程上下文）
                 vs_dc_handle_ltm_isr_work() → dc_hw_get_display_ltm2_data()
                   读 SD_RATIO / AVG_LUMA / AVG_WOPR / ROI_SUM，read_done = true
                 ↓ 用户态
                 dtest_get_ltm_hw_readout_result()
                   轮询 read_done（10 × 1ms）→ memcpy 到 dev->ltm_read_params
                 ↓ frame N+1
                 _map_drm_ltm() 把它作为 ReadParams 传给 algo
```

- **LTM 模式不走这条路**（中断里就被过滤），只靠 WDMA 直方图。
  所以 LTM 模式下 `dev->ltm_read_params` 一直是全 0，**这是预期行为**。
- 直方图不走这条路：硬件 WDMA 直接写用户态 BO，用户态 `bo_map()` 后自己解包。

### 2.5.5 disable 路径与 enable **不对称**

- `ltm2_hw_config()` 在 `enable == false` 时**只调 `ltm2_enable_config_hw()`**，
  子模块的使能位和所有系数/LUT **保持原值**，硬件靠总开关旁路整个模块。
- 对比 LTM1.0：`ltm_freq_decomp_config_hw()` / `ltm_tone_adj_config_hw()` /
  `ltm_dither_config_hw()` / `ltm_hist_cd_set_config_hw()` 都有完整 `else` 分支清 0。
  **LTM2.0 的对应函数没有**（只有 `ltm2_af_lut_config_hw()` 有个写 0 的 else，但见 2.9）。
- **含义**：LTM2.0 case 之间必须靠 reset 清状态，不能指望"关掉 LTM"把寄存器恢复干净；
  `--soft` / `--clean` / `--reinstall` 的选择会影响结果。
- `ltm2_enable_config_hw()` 在 disable 时把 `display->ltm2_mode` 强制回 `VS_LTM_LTM`，
  顺带关掉 2.5.4 的回读路径。

---

## 2.6 寄存器配置

### 2.6.1 总开关

| 寄存器 | 字段 | 含义 | 写在哪 |
|---|---|---|---|
| `DCREG_SH_PANEL{0,1}_CONFIG` | `LTM` | LTM 模块总开关（与 GTM 共用） | `ltm2_enable_config_hw()` |

### 2.6.2 `LTM2_CONFIG` 位图（shadow，读改写）

这是 LTM2.0 的功能开关总表，排查"某模块没生效"先 dump 这个寄存器。

| bit | 字段 | 含义 | 数据来源 | 写在哪个函数 |
|:-:|---|---|---|---|
| 0 | `FREQ_DECOMP` | 亮度频率分解 | `freq_decomp.decomp_enable` | `ltm2_freq_decomp_config_hw()` |
| 1 | `SLICE_AFFINE_GRID` | slice / affine grid | `af_slice.enable` | `ltm2_af_slice_config_hw()` |
| 2 | `TONE_ADJUST` | 全局 tone adjust | `tone_adj.enable` | `ltm2_tone_adj_config_hw()` |
| 3 | `COLOR_PROTECT` | 色彩保护 | `color_protect_enable` | `ltm2_enable_config_hw()` |
| 4 | `DITHER` | dither | `ltm_dither.dither_enable` | `ltm2_dither_config_hw()` |
| 5 | `SD_BLENDING` | scan detection blending | `ltm_sd.blending_enable` | `ltm2_sre_sd_config_hw()` |
| 6 | `DSCALER` | 下采样 | `ltm_ds.enable` | `ltm2_ds_config_hw()` |
| 7 | `CDETECTION` | content detection | `ltm_cd_set.enable` | `ltm2_hist_cd_set_config_hw()` |
| 8 | `LUMA_BLENDING` | 平均亮度计算 | `ltm_luma.luma_blending_enable` | `ltm2_luma_config_hw()` |
| 9 | `WOPR_BLENDING` | WOPR 模块 | `sre_wopr.blending_enable` | `ltm2_sre_wopr_config_hw()` |
| 10 | `HIST` | 直方图统计 | `ltm_hist_set.enable` | `ltm2_local_hist_set_config_hw()` |
| 11 | `HIST_OVERLAP` | 直方图 overlap | `ltm_hist_set.overlap` | 同上 |
| 12 | `NON_ROI_MAP_WAY` | 1 = 非 ROI 区取最近 block 映射 | `af_slice.non_roi_mapping_way` | `ltm2_af_slice_config_hw()` |
| 13 | `AD_TXT_JUD` | ROI 文本检测 | `ad_txt_jud_en` | `ltm2_enable_config_hw()` |
| 14 | `LOCAL_HIST_WDMA` | 局部直方图 WDMA | `ltm_hist_get.enable` | `ltm2_local_hist_get_config_hw()` |

> **`ltm2_luma_config_hw()` 的两个 enable 不是一回事**：函数形参 `enable` 来自
> `ltm_luma.enable`，只控制**是否写 RGB→Y 系数**；而 `LUMA_BLENDING` 位取的是
> `ltm_luma.luma_blending_enable`，**无论形参真假都会被写**。
> 其余子模块的使能位与形参 `enable` 一致。

### 2.6.3 其余寄存器

参数寄存器（系数、LUT、ROI、地址等）的逐项含义、类型、默认值、
以及 `struct drm_vs_ltm` 字段 → 寄存器位域的完整映射，见 **附录 C**。
shadow / non-shadow 的分类见 2.5.3，只读结果寄存器见 **附录 B**。

两个写入条件上的特例：

- **`DS_OUTPUT` / `ROIX` / `ROIY` / `ROI_LEN` 是无条件写的**（不管 `ltm_ds.enable`），
  只有两个 `DS_*_NORM` 受 enable 控制；
- `LTM2_LOCAL_HIST_WB_*ADDRESS` 仅 `ltm_hist_get.enable` 时写，
  地址由 `hist_bo_handle` 经 `vs_gem_object_lookup()` 换算成 iova。

---

## 2.7 设计需求

**需求来源**：`ltm_v2.0/` 下三份资料（见文末）。

**要解决的问题**：见 2.1.2。

**软件侧设计约束**：

- **算法在用户态**：algo team 交付源码 → 编译成 `libdpualgorithm.so` → drmtest 运行时
  `dlopen`。驱动的职责边界就是 `struct drm_vs_ltm` 这个 UAPI 结构体。
- **单次下发**：整套参数（含 1088 项 LUT）必须一个 blob 一次性给，不能拆多个 property，
  否则跨帧反馈的一致性无法保证。
- **跨帧反馈**：必须提供稳定回读通道（`LTM_HW_READOUT` + `read_done` + WDMA BO）。
- **规模限制**：grid ≤ 8×8，affine LUT 固定 1088 项，直方图 64 bin × 64 block。
- **不做参数兜底**：驱动只做少量边界 check，参数合理性由算法库负责。

**明确不支持**：

- `ltm2_mask == 0` 的芯片不支持 SRE / FOSS；
- LTM2.0 上不支持 `LTM_AF_FILTER` property；
- LTM 只挂 PANEL0 / PANEL1（寄存器都用 `VS_SET_PANEL01_FIELD`）；
- **CMD_LIST 模式下 tone_adjust / dither / affine LUT 尚未实现**（见 2.9）；
- **vcmd 路径只覆盖 LTM1.0**，没有 SRE / FOSS 分支（见 2.9）；
- LTM2.0 不复用 LTM1.0 的 degamma / gamma / luma_adj / af_trans / color / luma_ave_set
  子模块（完整清单见附录 C.10）。

---

## 2.8 软件适配

### 2.8.1 算法配置流程

**驱动不含算法。** 软件侧要做的是：① 提供 `.so` 被调用的 API 接口声明；
② 在接口实现里完成对底层算法的调用；③ 把算法处理后的参数配置给相应寄存器。
算法输入 = **用户配置（case 参数）** + **上一帧从寄存器/内存读回的参数**。

```
用户态 (drmtest + libdpualgorithm.so)
  ① JSON case "LTM": { ltm_enable, ltm2_mode, Grid_num, width, height,
                       Strength, temporal_smooth, VidSetting{...} ... }
         ↓ get_ltm_params_from_json()
  ② struct ltm_algo_params p
         +
  ③ 上一帧回读： dev->ltm_read_params (APB: avg_sd/avg_luma/avg_wopr/sum_txt)
                 hist_buf[64][64]     (WDMA BO)
                 cdf_curve_last[][]   (算法内部跨帧状态)
         ↓
  ④ dpu_algo_api_ltm / _ltm_sre / _ltm_foss     ← 按 ltm2_mode 分派
         ↓
  ⑤ struct drm_vs_ltm（含 1088 项 affine LUT）
         ↓ drmModeCreatePropertyBlob + atomic set
内核 (vs_drm.ko)
  ⑥ ltm_check() → ltm_config_hw() → ltm2_hw_config()      按 ltm2_mask 分流
  ⑦ dc_write / dc_write_non → LTM2_* 寄存器
  ⑧ frame done 中断 → vs_dc_handle_ltm_isr_work() → 回填 LTM_HW_READOUT，read_done = 1
         ↓ 下一帧回到 ③
```

**分步要点**

**① → ②　解析（`get_ltm_params_from_json()`）**

- 先设一批**代码内默认值**（`Sensity = 0.4`、`IIR_Wgt_max = 0.9`、
  `spatial_kernal = {164,101,60,23,14,3}`、`foss_strength = 1`、
  `luma_luma_intensity = 64`）—— 注意与 xlsx 标注的默认值不完全一致，见附录 A；
- `ltm2_mode` 是**字符串**（`"LTM"` / `"SRE"` / `"FOSS"`），**不认识的值静默回落到 `"LTM"`**；
- 只有 `ltm_enable` 为真才解析其余字段；`globalDisable` 为真则整个 LTM 不下发。

**③　取上一帧数据（两条独立通路）**

| 通路 | 数据 | 怎么来 | 何时可用 |
|---|---|---|---|
| **APB** | `avg_sd` / `avg_luma` / `avg_wopr` / `sum_txt` | 驱动在 frame done worker 里读寄存器，回填 `LTM_HW_READOUT` | 用户态轮到 `read_done == true` 后 |
| **WDMA** | `histogram` | 硬件 DMA 直接写用户态 BO | wb frame done 触发，代表已写完 |

**④　按 mode 分派**

```c
if (!ltm2_mask || ((ltm2_mask & BIT(VS_LTM_LTM)) && p.ltm2_mode == VS_LTM_LTM))
        dpu_algo_ltm(...,  spatial_kernal, hist_buf, cdf_curve_last, &ltm);
else if ((ltm2_mask & BIT(VS_LTM_SRE)) && p.ltm2_mode == VS_LTM_SRE)
        dpu_algo_ltm_sre(..., cdf_curve_last, luma_luma_intensity,
                         &read_params, &ltm);            /* 无 hist */
else if ((ltm2_mask & BIT(VS_LTM_FOSS)) && p.ltm2_mode == VS_LTM_FOSS)
        dpu_algo_ltm_foss(..., hist_buf, cdf_curve_last, foss_strength,
                          foss_uniform_blend_strength, &read_params, &ltm);
```

`dlsym` 没拿到符号时打印 `Not found the LTM ... algo function!` 并返回 0
（**不下发 property**，这一帧没有 LTM）。

**⑤　算法输出后，软件只补三个算法不管的字段**

```c
ltm.ltm_hist_get.enable         = (p.ltm2_mode != VS_LTM_SRE);
ltm.ltm_hist_get.fd             = dev->fd;
ltm.ltm_hist_get.hist_bo_handle = dev->ltm_hist_bo1->handle;  /* 下一帧 WDMA 目标 */
ltm.ltm2_mode                   = p.ltm2_mode;                /* 驱动据此决定回读 */
```

其余字段（各子模块 `enable` + 系数 + LUT）全部由算法填。
**驱动不做默认值兜底** —— 某模块没生效，先确认算法有没有把对应 `enable` 置起来
（各 mode 的期望使能状态见附录 C.0）。

### 2.8.2 用户态怎么设这个 property

```c
struct drm_vs_ltm ltm = { 0 };
uint32_t blob_id = 0;

/* 1. 调算法库填充 ltm（见 2.8.1 ④）*/
/* 2. 补 ltm_hist_get / ltm2_mode（见 2.8.1 ⑤）*/
/* 3. 建 blob，atomic set 到 CRTC 的 "LTM" property */
drmModeCreatePropertyBlob(dev->fd, &ltm, sizeof(struct drm_vs_ltm), &blob_id);
```

**关闭 LTM**：把 `"LTM"` 设成 **0**（blob id = 0），驱动走 `!new_data` 分支置
`enable = false`。

### 2.8.3 algo .so 接口

路径 `LIB_DPU_ALGORITHM_PATH = "./../dpu-algo/lib/libdpualgorithm.so"`，
符号在 `dpu_algorithm_lib_init()` 里用 `dlsym` 取。

| 函数指针 | dlsym 名 | 除公共参数外还要 |
|---|---|---|
| `dpu_algo_ltm` | `dpu_algo_api_ltm` | `mode`、`Strength`、`spatial_kernal`、`hist_buf` |
| `dpu_algo_ltm_sre` | `dpu_algo_api_ltm_sre` | `luma_luma_intensity`、`ReadParams` |
| `dpu_algo_ltm_foss` | `dpu_algo_api_ltm_foss` | `Strength`、`spatial_kernal`、`hist_buf`、`foss_strength`、`foss_uniform_blend_strength`、`ReadParams` |

公共参数：`fr`（帧号）、`temporal_smooth`、`layerWidth/Height`、`gridWidth/Height`、
`sensitivity`、`iir_max_wgt`、`cdf_crv_last`（跨帧状态，in/out）、`hw_config`（输出）。

> **`hw_config` 和 `ReadParams` 都是 `void *`**，编译期不检查类型。
> `struct drm_vs_ltm` / `struct drm_vs_ltm_hw_readout` 的布局是驱动与 algo 的
> **隐式 ABI 契约** —— 改 UAPI 结构体必须同步 algo，否则静默写坏内存。

`dpu-algo` 在本 workspace 中不存在（可选组件），没有 `.so` 时 LTM 不下发。

### 2.8.4 测试 case

JSON 里 LTM 是 CRTC 的一个 property，值是**算法输入参数**（不是寄存器值）。
字段含义见附录 A。

```python
p_ltm = OrderedDict()
p_ltm["ltm_enable"]      = 1
p_ltm["ltm2_mode"]       = 'LTM'        # 'LTM' | 'SRE' | 'FOSS'
p_ltm["Grid_num"]        = [8, 8]       # [w, h], <= 8
p_ltm["width"]           = 1920
p_ltm["height"]          = 1280
p_ltm["temporal_smooth"] = 1
p_ltm["Strength"]        = 2            # LTM / FOSS；SRE 不用
p_ltm["foss_strength"]                = 1      # FOSS only
p_ltm["foss_uniform_blend_strength"]  = 0.0    # FOSS only
p_ltm["luma_luma_intensity"]          = 64     # SRE only
p_ltm["VidSetting"] = OrderedDict([
    ("Sensity",      0.4),
    ("IIR_Wgt_max",  0.9),
    ("Spatial_Coef", [380, 101, 60, 0, 0, 0]),
])
return OrderedDict([('LTM', p_ltm)])
```

每个 case 必须是 `DTestFrameUnit`（`ltm2.py` 里是 3 帧 `normal` / 9 帧 `scene_change`），
资源是连续帧序列 `imgseq2_f_0.bmp` ~ `imgseq2_f_8.bmp`。

```sh
python3 script/dtest.py -g --cid 0x2000003d
python3 script/dtest.py -d auto_cases/0x2000003d -o /tmp/dump --soft -f '*crtc0*postpq*ltm*'
```

### 2.8.5 平台 / 版本差异

| 维度 | 差异 |
|---|---|
| LTM1.0 vs 2.0 | `ltm_config_hw()` 按 `info->ltm2_mask` 运行时分流；两套 `*_config_hw` 完全独立，寄存器前缀 `LTM_` vs `LTM2_` |
| register set | LTM2 寄存器由 `#ifdef DCREG_SH_PANEL0_LTM2_CONFIG_Address` 保护，老 register set 下整块不编译 |
| APB vs VCMD | `map_drm_ltm()` 分流；vcmd 走 `_map_drm_ltm_vcmd()`（双 BO 乒乓 + `DRM_IOCTL_VS_SET_CTX` 查 `frm_exe_count` 同步 + `update_ltm_slope_bias()` 回填 N+2 帧）。**vcmd 路径只支持 LTM1.0** |
| CMD_LIST | shadow 寄存器走 `dc_write_lut()` 批量写；但 LTM2.0 的 `tone_adj` / `dither` / `af_lut` 的 cmd_list 分支未实现 |
| QEMU | `dev->use_vcmd && !dev->is_qemu` 才走 vcmd |
| kernel 版本 | `< 5.19` 手动遍历 `mode_obj->properties[]` 找 blob id，`>= 5.19` 用 `drm_object_property_get_default_value()` |

### 2.8.6 与其它 feature 的关系

| 关系 | 说明 |
|---|---|
| **LTM ↔ GTM** | 共用 `PANEL_CONFIG.LTM` 总开关和 `ltm_enable_config_hw()`；`display_info->ltm` / `->gtm` 通常互斥 |
| **LTM ↔ LTM_AF_FILTER** | LTM2.0 下被 check 拒绝，affine 系数改走 `ltm_af_luts` |
| **LTM ↔ HISTOGRAM** | 两个独立 feature，寄存器和 property 都不共用 |
| **LTM ↔ SCALE / window box** | `display_commit()` 里 LTM 先于 `display_set_window_box()`；LTM 的 ROI / DS 尺寸需与 SCALER 之后的实际 panel 尺寸对齐，但**驱动不校验** |
| **LTM ↔ non-shadow 缓存** | 大量 `dc_write_non` 占用共享的 `non_shadow_reg_cache[display_id]`，与其它用 non-shadow 的 feature 抢容量 |

---

## 2.9 已知问题 / 限制 / TODO

| # | 问题 | 位置 | 状态 |
|:-:|---|---|---|
| 1 | **CMD_LIST 下 `tone_adj` 未实现**：`use_cmd_list` 分支为空（`VIV_TODO@Pei Li: Add CMD_LIST flow`），tone adjust LUT 完全不下发 | `ltm2_tone_adj_config_hw()` | Open |
| 2 | **CMD_LIST 下 `dither` 未实现**：同上 | `ltm2_dither_config_hw()` | Open |
| 3 | **CMD_LIST 下 `af_lut` 无分支**：只在 `if (!hw->use_cmd_list)` 里写，cmd_list 模式下**核心映射曲线一个都不写**，且无 TODO 注释 | `ltm2_af_lut_config_hw()` | Open |
| 4 | **`af_lut` enable / disable 循环次数不一致**：enable 硬编码 64 组 × 17 项（544 次写），disable 用 `af_lut->entry_cnt` 且步长 2。不符时硬件自增地址错位 | `ltm2_af_lut_config_hw()` | Open |
| 5 | **`af_lut` 忽略 `entry_cnt`**：enable 路径完全不看它，算法给多少项都按 1088 写 | `ltm2_af_lut_config_hw()` | Open |
| 6 | **返回类型不匹配**：函数声明 `bool`，出错时 `return -ENOENT`（隐式转 `true`），调用方无法感知 GEM lookup 失败 | `ltm_local_hist_get_config_hw()`、`ltm2_local_hist_get_config_hw()` | Open |
| 7 | **disable 不清子模块寄存器**：缺 LTM1.0 那样的 `else` 清零分支，case 间靠 reset 清状态 | 见 2.5.5 | 设计如此，case 层注意 |
| 8 | **不校验 ROI / DS 尺寸**：`obj_state` 未使用，ROI 越界或 DS output 与 panel 不匹配不会被挡住 | `ltm_check()` | Open |
| 9 | **`ltm2.py` golden 为空**：`__get_golden()` 返回空字典，所有 LTM2.0 case 都没有 md5 golden | ltm2.py | Open |
| 10 | **vcmd 路径不支持 SRE / FOSS**：只调 `dpu_algo_ltm`，无 mode 分派，vcmd 下跑 SRE/FOSS 会按 LTM 处理 | `_map_drm_ltm_vcmd()` | Open |
| 11 | **回读超时只打日志不报错**：超时后**保留上一帧的 `ltm_read_params` 继续用**，算法输入静默变旧、结果不可复现 | `dtest_get_ltm_hw_readout_result()` | Open |
| 12 | **`"LTM_ENABLE"` 是死表项**：`prop_mapper.c` 登记了它，但驱动从未创建同名 property（`ltm_enable` 只是 `"LTM"` blob 内部的 JSON 字段） | prop_mapper.c | Open（无害，建议清理） |
| 13 | **结构体体积大**：`sizeof(struct drm_vs_ltm) == 51188`，其中 LTM2.0 用不到的 `ltm_hist_get`（36876B）+ `af_filter`（9220B）≈ 46KB。每帧都要拷两次 | vs_drm_api.h | Open（优化项） |

### 排查入口（按接口层顺序）

1. **`state->dirty` / `state->enable`** —— `"LTM"` 是 L3，没 dirty 就不写寄存器。
   在 `display_commit()` 的 states 循环里打点。
2. **`ltm_check()` 是否拒绝** —— dmesg 里找
   `The LTM is not support mode %d` / `grid width/height ... not meet the limit`。
   check 失败会让整个 atomic commit 失败。
3. **能力位** —— `ltm2_mask` / `display_info->ltm` 为 0 时 property 根本不创建，
   用户态 set 会找不到。用 `modetest -p` 或 `DC_INFO` blob 确认。
4. **走的是 LTM2 还是 LTM1 分支** —— 看 `ltm_config_hw()` 里 `ltm2_mask` 的取值。
5. **dump `LTM2_CONFIG`** —— 15 个使能位一次看清哪个模块没开（位图见 2.6.2）。
6. **区分 shadow / non-shadow** —— shadow 在 commit 时就写了，non-shadow
   （含 `AFFINE_LUT`）要等 frame done 才 flush。抓寄存器序列时注意这个时间差。
7. **回读不来** —— 先看 `display->ltm2_mode` 是不是 `VS_LTM_LTM`（那样中断里就不
   schedule work），再看 `read_done` 是否被清了但 worker 没跑。
8. **算法输入不对** —— 在 `_map_drm_ltm()` 里打印 `struct ltm_algo_params` 和
   `read_params`，确认 JSON 解析和上一帧回读都正常。

---

## 2.10 Change History

| 日期 | 版本 | 修改内容 | 修改人 |
|---|---|---|---|
| 2026-09-10 | v1.0 | 初版：按模板第二节整理 LTM2.0 软件配置文档 | Yongjian.rao |
| 2026-09-11 | v1.1 | 补充功能构成 / 设计目的 / 算法配置流程；新增附录 A 用户参数、附录 B 读回参数、附录 C 硬件参数含义 | Yongjian.rao |
| 2026-09-11 | v1.2 | 去掉所有代码行号（改为文件名 + 函数名）；精简正文，新增"快速上手"；合并 2.6 重复的寄存器表到附录 C | Yongjian.rao |

---

## 附录 A：用户参数（case → 算法的输入）

写在 CRTC 的 `"LTM"` 对象下，由 `get_ltm_params_from_json()` 解析进
`struct ltm_algo_params`。**这些是算法输入，不是寄存器值。**

### A.1 通用参数

| JSON 字段 | 结构体字段 | 类型 | 取值 / 默认 | 适用 | 含义 |
|---|---|---|---|---|---|
| `ltm_enable` | `enable` | bool | 0 / 1 | 全部 | 是否启用。为 0 时**不下发 property**，其余字段不解析 |
| `ltm2_mode` | `ltm2_mode` | **字符串**→`u32` | `"LTM"` / `"SRE"` / `"FOSS"`<br>默认 `"LTM"` | 仅 2.0 | 选模式。决定算法入口、要不要直方图、驱动回读哪些寄存器。**不认识的字符串静默回落到 `"LTM"`** |
| `Grid_num` | `grid_width`<br>`grid_height` | `2 × u32` | `[1, 8]`<br>ltm2.py 用 `[8,8]` | 全部 | `[宽方向 block 数, 高方向 block 数]`。**2.0 上限 8×8**（1.0 是 12×12），超限被 `ltm_check()` 拒绝 |
| `width` / `height` | `layer_width`<br>`layer_height` | `u32` | = layer 宽高 | 全部 | layer 尺寸，算法据此算 grid 缩放系数 |
| `temporal_smooth` | `temporal_smooth` | bool（写 0/1） | 默认 0 | 全部 | 是否启用帧间时域平滑（IIR） |
| `Strength` | `strength` | `float` | `[1, 20]`<br>default 2，**suggest 2-8** | LTM、FOSS | LTM 强度。越大局部增强越猛，过大易出光晕/放大噪声。**1.0 是 `u32`，2.0 是 `float`** |
| `Mode` | `mode` | `u32` | 0 / 1 | **仅 1.0** | 0 = 用用户配的 strength/grid；1 = 用推荐参数 |
| `vid_mode` | —— | `u32` | — | **仅 1.0** | 是否启用 video mode。`ltm_algo_params` 里**没有这个字段**，2.0 不解析 |

### A.2 模式专用参数

| JSON 字段 | 结构体字段 | 类型 | 默认 | 适用 | 含义 |
|---|---|---|---|---|---|
| `luma_luma_intensity` | 同名 | `u32`（xlsx 标 double，**代码是 `uint32_t`**） | 64 | **SRE** | 亮度增强强度。SRE 的主强度旋钮（SRE 不用 `Strength`） |
| `foss_strength` | 同名 | `float` | 1 | **FOSS** | FOSS 保真强度 |
| `foss_uniform_blend_strength` | 同名 | `float` | 0.0 | **FOSS** | 均匀区域混合强度，0 = 不混合。**xlsx 里拼成了 `...strenth`（漏 g），以代码为准** |

### A.3 `VidSetting` 子对象（帧间平滑 / 空间滤波）

可选；**整个 `VidSetting` 缺省时用代码内默认值**（不是 xlsx 的 default）。

| JSON 字段 | 结构体字段 | 类型 | xlsx 建议 | **代码默认** | 适用 | 含义 |
|---|---|---|---|---|---|---|
| `Sensity` | `sensitivity` | `double` | `[0,1]`，suggest 0.4-0.8，default 0.5 | **0.4** | 全部 | IIR 对相邻两帧差异的敏感度。**越小越敏感**（跟画面变化越快，闪烁风险升高；越大越稳但响应慢） |
| `IIR_Wgt_max` | `iir_max_wgt` | `double` | `[0,1]`，suggest 0.5-1.0，default 0.8 | **0.9** | 全部 | IIR 混合上一帧与当前帧时**当前帧的最大权重**。1.0 = 完全用当前帧（等于不平滑） |
| `Spatial_Coef` | `spatial_kernal` | `u16[6]` | 1×6，**系数和 = 1024** | **`{164,101,60,23,14,3}`**<br>ltm2.py 用 `{380,101,60,0,0,0}` | LTM、FOSS | 空间滤波系数。SRE 不用（`dpu_algo_api_ltm_sre` 入参里没有） |

> **`Sensity` / `IIR_Wgt_max` 的 xlsx default 与代码 default 不一致**
> （0.5 vs 0.4、0.8 vs 0.9）。写 case 时**显式指定**这两个值，别依赖默认。

另有一个调试用字段 **`globalDisable`**：置 1 可在不删 property 的前提下跳过整个 LTM
（配置侧和回读侧都识别）。

---

## 附录 B：读回参数（硬件 → 算法的反馈）

两条**完全独立**的通路，容易混淆：

| | 通路 1：APB 寄存器 | 通路 2：WDMA 内存 |
|---|---|---|
| 数据 | `avg_sd` / `avg_luma` / `avg_wopr` / `sum_txt` | `histogram` |
| 载体 | `struct drm_vs_ltm_hw_readout`（200B）经 `LTM_HW_READOUT` blob | 用户态 dumb BO（`dev->ltm_hist_bo1`） |
| 读取方式 | 驱动在 frame done worker 里 `dc_read_immediate()` 逐个读 | 硬件 DMA 直接写内存，软件 `bo_map()` 后自己解包 |
| 写回时机 | **frame done 之后** | **wb frame done 触发**，代表已写完 |
| 有效标志 | `read_done` | 无，靠帧同步 |
| 哪些 mode 用 | **仅 SRE / FOSS** | **仅 LTM / FOSS** |

### B.1 APB 回读（`struct drm_vs_ltm_hw_readout`）

由 `dc_hw_get_display_ltm2_data()` 填充。

| xlsx 名 | 字段 | 类型 | 寄存器 | LTM | SRE | FOSS | 含义 |
|---|---|---|---|:-:|:-:|:-:|---|
| `SD_RATIO` | `avg_sd[64]` | `u8 × 64` | `LTM2_SD_RATIO_DATA` | ❌ | ✅ | ✅ | 每 block 的 scan-detection 命中比例，算法据此定 SD blending 力度 |
| `LUMA_AVG` | `avg_luma[64]` | `u8 × 64` | `LTM2_AVG_LUMA_DATA` | ❌ | ✅ | ✅ | 每 block 平均亮度。需 `LUMA_BLENDING` 使能 |
| `WOPR_AVG` | `avg_wopr[64]` | `u8 × 64` | `LTM2_AVG_WOPR_DATA` | ❌ | ✅ | ❌ | 每 block 的白底占比均值。需 `WOPR_BLENDING` 使能 |
| `Sum roi` | `sum_txt` | `u32` | `LTM2_ROI_SUM_DATA` | ❌ | ❌ | ✅ | ROI 文本判定累加值。需 `AD_TXT_JUD` 使能 |
| — | `read_done` | bool | 软件标志 | — | — | — | 驱动→用户态的数据有效标志 |

三个读取细节：

- **同地址自增数据口**：循环 `entry_cnt += 4`、**每次读同一地址**，一个 u32 拆 4 个 u8
  → 64 个 block 只需 16 次读；
- 用 `dc_read_immediate()`，绕过 regmap 缓存直接读 non-shadow 域；
- **LTM 模式下函数直接 `return`**，`ltm_read_params` 一直全 0（预期行为）。

> **坑（2.9 第 11 条）**：轮询超时只打印 timeout，**但保留上一帧的值继续用**。

### B.2 WDMA 直方图

| 名 | 用户态载体 | 类型 | LTM | SRE | FOSS | 含义 |
|---|---|---|:-:|:-:|:-:|---|
| `histogram` | `hist_buf_first[][64]` | `u16 × 64 × 64` | ✅ | ❌ | ✅ | 每 block 的亮度直方图，每 block `HIST_GRID_DEPTH` 个 bin（默认 64） |

| | LTM1.0 | LTM2.0 |
|---|---|---|
| block 数 | 144（12×12） | **64（8×8）** |
| BO 创建 | `bo_create_dumb(fd, 144, 32, 32, ...)` | `bo_create_dumb(fd, 64, 32, 32, ...)` |
| BO 个数 | vcmd 路径 2 个（乒乓） | APB 路径 1 个 |

```c
for (i = 0; i < max_grid_num; i++)          /* LTM2.0: 64 */
        for (j = 0; j < 32; j++) {          /* 32 个 u32 = 64 个 u16 bin */
                hist_buf_first[i][2*j]     = hist_arr[i*32 + j] & 0xffff;
                hist_buf_first[i][2*j + 1] = hist_arr[i*32 + j] >> 16;
        }
```

> `hist_buf_first[144][64]` / `cdf_curve_last[144][65]` 按 LTM1.0 的 144 维声明，
> LTM2.0 只用前 64 行 —— 刻意的兼容写法，不是 bug。

### B.3 第三份跨帧状态：`cdf_curve_last`

不是硬件回读，但同样是跨帧数据，容易漏：

| 载体 | 类型 | 谁维护 | 含义 |
|---|---|---|---|
| `cdf_curve_last[144][65]` | `u32` | **算法内部**（软件只负责一直传同一块内存） | 上一帧 CDF 曲线，IIR 平滑的状态。`static` 数组，进程内持久 |

因为是 `static`，**同一个 drmtest 进程内跨 case 不清零** ——
前一个 case 的收敛状态会带到下一个。这是 LTM2.0 case 要注意执行顺序 /
配合 `--reinstall` 的原因之一。

---

## 附录 C：硬件参数含义（算法输出 → 寄存器）

来源：`ltm_v2.0/寄存器配置与用户参数.xlsx`，补上对应的 `struct drm_vs_ltm` 字段和寄存器。
类型记法 `Ua.b` = 无符号定点，a 位整数 + b 位小数（`U0.10` = 10 位纯小数）。
"（算）"= 由算法根据分辨率/grid 计算，无固定默认值。

### C.0 各 mode 的期望使能状态

排查"某模块没生效"时的对照表 —— 这些位由**算法**填进 `struct drm_vs_ltm`，驱动不兜底。

| 使能位 | `drm_vs_ltm` 字段 | LTM | SRE | FOSS |
|---|---|:-:|:-:|:-:|
| `ENABLE` | proto `state->enable` | 1 | 1 | 1 |
| `ENABLE_FREQ_DECOMP` | `freq_decomp.decomp_enable` | 1 | 1 | 1 |
| `ENABLE_SLICE_AFFINE_GRID` | `af_slice.enable` | 1 | 1 | 1 |
| `ENABLE_HIST` | `ltm_hist_set.enable` | 1 | 1 | 1 |
| `ENABLE_HIST_OVERLAP` | `ltm_hist_set.overlap` | 1 | 1 | 1 |
| `NON_ROI_MAPPING_WAY` | `af_slice.non_roi_mapping_way` | 1 | 1 | 1 |
| `ENABLE_DITHER` | `ltm_dither.dither_enable` | 1 | 1 | 1 |
| `ENABLE_DS` | `ltm_ds.enable` | 1 | 1 | 1 |
| `ENABLE_CONTENT_DETECTION` | `ltm_cd_set.enable` | 1 | 1 | 1 |
| `ENABLE_COLOR_PROTECT` | `color_protect_enable` | 1 | 1 | 1 |
| `LOCAL_HIST_WDMA` | `ltm_hist_get.enable` | 1 | 1 | 1 |
| `af_lut.enable` | `ltm_af_luts.enable` | 1 | 1 | 1 |
| `grid_size.enable` | `grid_size.enable` | 1 | 1 | 1 |
| `LUMA_BLENDING_ENABLE` | `ltm_luma.luma_blending_enable` | **0** | 1 | 1 |
| `SD_BLENDING_ENABLE` | `ltm_sd.blending_enable` | **0** | 1 | 1 |
| `WOPR_BLENDING_ENABLE` | `sre_wopr.blending_enable` | **0** | 1 | **0** |
| `AD_TXT_JUD_EN` | `ad_txt_jud_en` | **0** | **0** | 1 |
| `ENABLE_TONE_ADJUST` | `tone_adj.enable` | **0** | **0** | **0** |

### C.1 总开关与亮度定义

| 参数 | 类型 | 默认 | 含义 | 字段 | 寄存器 |
|---|---|---|---|---|---|
| `ENABLE` | U1.0 | 1 | 模块总开关 | proto `state->enable` | `PANEL_CONFIG.LTM` |
| `RGBY_COEFF[3]` | U0.10 | 306, 601, 119 | RGB→Y 系数，`[0,1023)`，默认即 BT.601 权重 | `ltm_luma.coef[0..2]` | `LTM2_RGBY_COEF.COEF0/1/2` |
| `GRAY_WEIGHT` | U1.6 | 50 | gray 与 lightness 混合时 Y 的权重。**0 = 全用 lightness，64 = 全用 gray**，`[0,64]` | `ltm_luma.coef[4]` | `LTM2_GRAY_LIGHT_WGT.GRAY_WEIGHT` |
| `LIGHTNESS_WEIGHT` | U1.6 | 48 | lightness 在 RGB min / max 之间的权重。**0 = RGB min，64 = RGB max** | `ltm_luma.coef[3]` | `LTM2_GRAY_LIGHT_WGT.LIGHTNESS_WEIGHT` |
| `LUMA_BLENDING_ENABLE` | U1.0 | 1 | 平均亮度计算使能（`avg_luma` 回读的前提） | `ltm_luma.luma_blending_enable` | `LTM2_CONFIG.LUMA_BLENDING` |
| `ENABLE_COLOR_PROTECT` | U1.0 | 1 | color mapping 阶段色彩保护 | `color_protect_enable` | `LTM2_CONFIG.COLOR_PROTECT` |

### C.2 频率分解

| 参数 | 类型 | 默认 | 字段 | 寄存器 |
|---|---|---|---|---|
| `ENABLE_FREQ_DECOMP` | U1.0 | 1 | `freq_decomp.decomp_enable` | `LTM2_CONFIG.FREQ_DECOMP` |
| `FREQ_DECOMP_COEF_0..3` | U8.0 | 0, 8, 16, 0 | `freq_decomp.coef[0..3]` | `LTM2_FREQ_DECOMP_COEF_A.COEF0..3` |
| `FREQ_DECOMP_COEF_4..5` | U8.0 | 16, 32 | `freq_decomp.coef[4..5]` | `LTM2_FREQ_DECOMP_COEF_B.COEF4/5` |

> `coef[]` 声明为 9 项（1.0 用 3×3 滤波），**LTM2.0 只用前 6 项**；`norm` 字段 2.0 不用。

### C.3 分块与仿射网格

| 参数 | 类型 | 默认 | 含义 | 字段 | 寄存器 |
|---|---|---|---|---|---|
| `ENABLE_SLICE_AFFINE_GRID` | U1.0 | 1 | slice / affine 使能 | `af_slice.enable` | `LTM2_CONFIG.SLICE_AFFINE_GRID` |
| `GRID_WIDTH` / `GRID_HEIGHT` | U4.0 | 8 / 8 | 水平 / 垂直 block 数（`[1,8]`） | `grid_size.width/height` | `LTM2_GRID_CONFIG.GRID_WIDTH/HEIGHT` |
| `GRID_DEPTH` | U5.0 | 17 | affine grid 深度（每 block 曲线控制点数） | `grid_size.depth` | `LTM2_GRID_CONFIG.GRID_DEPTH` |
| `HIST_GRID_DEPTH` | U7.0 | 64 | 直方图 bin 数 | `grid_size.hist_depth` | `LTM2_GRID_CONFIG.HIST_GRID_DEPTH` |
| `GRID_X_SCALE` / `GRID_Y_SCALE` | U16.0 | （算） | grid 缩放归一化系数 | `af_slice.scale[0/1]` | `LTM2_GRID_SCALE.XSCALE/YSCALE` |
| `NON_ROI_MAPPING_WAY` | U1.0 | 1 | **1 = 非 ROI 区用最近 block 映射** | `af_slice.non_roi_mapping_way` | `LTM2_CONFIG.NON_ROI_MAP_WAY` |
| `BLOCK_RATIO_HR` | U27.0 | （算） | 高分辨率 block ratio | `af_slice.block_ratio_hr` | `LTM2_BLOCK_RATIO_HR` |
| `BLOCK_RATIO_LR` | U27.0 | （算） | 低分辨率 block ratio | `ltm_hist_set.block_ratio_lr` | `LTM2_BLOCK_RATIO_LR` |
| `AFFINE_LUT[1088]` | 1088 × U11.0 | （算） | **核心映射曲线**，64 组 × 17 项；两项拼一个 32bit，每组最后一项单独配（`VALUE1 = 0`） | `ltm_af_luts.ltm_af_lut[]` | `LTM2_AFFINE_LUT.VALUE0/VALUE1`（同地址自增） |

### C.4 直方图统计

| 参数 | 类型 | 默认 | 含义 | 字段 | 寄存器 |
|---|---|---|---|---|---|
| `ENABLE_HIST` | U1.0 | 1 | 直方图统计使能 | `ltm_hist_set.enable` | `LTM2_CONFIG.HIST` |
| `ENABLE_HIST_OVERLAP` | U1.0 | 0 | 相邻 block 统计区重叠，减少 block 边界跳变 | `ltm_hist_set.overlap` | `LTM2_CONFIG.HIST_OVERLAP` |
| `HIST_OVERLAP_RATIO` | U0.10 | 0 | overlap 比例 | `ltm_hist_set.hist_overlap_ratio` | `LTM2_HIST_OVERLAP_RATIO` |
| `HIST_GRID_X/Y_SCALE` | U16.0 | （算） | 直方图 grid 缩放 | `ltm_hist_set.grid_scale[0/1]` | `LTM2_HIST_GRID_SCALE.XSCALE/YSCALE` |
| `WDMA ENABLE` | U1.0 | 1 | 局部直方图 WDMA 使能 | `ltm_hist_get.enable` | `LTM2_CONFIG.LOCAL_HIST_WDMA` |
| — | — | — | WDMA 目标地址（低/高 32bit），由 BO handle 换算 | `ltm_hist_get.hist_bo_handle` → iova | `LTM2_LOCAL_HIST_WB_ADDRESS`<br>`LTM2_LOCAL_HIST_WB_HIGH_ADDRESS` |

> `ltm_hist_set.grid_depth` / `start_pos[]` 是 1.0 字段，2.0 不用
> （bin 数改由 `grid_size.hist_depth` 走 `LTM2_GRID_CONFIG`）。

### C.5 内容检测

| 参数 | 类型 | 默认 | 字段 | 寄存器 |
|---|---|---|---|---|
| `ENABLE_CONTENT_DETECTION` | U1.0 | 1 | `ltm_cd_set.enable` | `LTM2_CONFIG.CDETECTION` |
| `CONTENT_DETECTION_COEF0..3` | U8.0 | 8, 16, 16, 32 | `ltm_cd_set.coef[0..3]` | `LTM2_CD_COEF.COEF0..3` |
| `CONTENT_DETECTION_THRESHOLD0..3` | U8.0 | 1, 12, 64, 128 | `ltm_cd_set.thresh[0..3]` | `LTM2_CD_THR.THR0..3` |
| `CONTENT_DETECTION_SLOPE0..1` | U1.14 | 1489, 256 | `ltm_cd_set.slope[0..1]` | `LTM2_CD_SLOPE.SLOPE0/1` |
| `CONTENT_DETECTION_MIN_WGT` | U2.0 | 2 | `ltm_cd_set.min_wgt` | `LTM2_CD_MIN_WGT` |

> xlsx 里 `CONTENT_DETECTION_MIN_WGT` 的描述写的是 *"Luma frequency decomposition
> filter coefficient 5"* —— 复制粘贴错误，按字段名理解为"内容检测最小权重"。
> `ltm_cd_set.filt_norm` / `.overlap` 是 1.0 字段，2.0 不写。

### C.6 下采样与 ROI

| 参数 | 类型 | 默认 | 字段 | 寄存器 |
|---|---|---|---|---|
| `ENABLE_DS` | U1.0 | 1 | `ltm_ds.enable` | `LTM2_CONFIG.DSCALER` |
| `DS_OUTPUT_WIDTH/HEIGHT` | U10.0 | （算） | `ltm_ds.output.w/h` | `LTM2_DS_OUTPUT.WIDTH/HEIGHT` |
| `DS_HOR/VER_SCALE_NORM` | U16.0 | （算） | `ltm_ds.h_norm` / `v_norm` | `LTM2_DS_HOR_NORM` / `LTM2_DS_VER_NORM` |
| `ROI_X_START` / `ROI_Y_START` | U13.0 | 0 / 0 | `ltm_ds.roi_x_start` / `roi_y_start` | `LTM2_ROIX.START` / `LTM2_ROIY.START` |
| `ROI_X_END` / `ROI_Y_END` | U13.0 | （算） | `ltm_ds.roi_x_end` / `roi_y_end` | `LTM2_ROIX.END` / `LTM2_ROIY.END` |
| `ROI_X_LEN` / `ROI_Y_LEN` | U13.0 | （算） | `ltm_ds.roi_x_len` / `roi_y_len` | `LTM2_ROI_LEN.XLEN/YLEN` |

> `ltm_ds.crop_l/r/t/b` 是 1.0 字段（写 `LTM_DS_CROP_HOR/VER`），2.0 用 ROI 取代。
> **`DS_OUTPUT` / `ROIX` / `ROIY` / `ROI_LEN` 无条件写**，只有两个 `NORM` 受 enable 控制。

### C.7 SRE 专用：WOPR 与 Scan Detection

| 参数 | 类型 | 默认 | 含义 | 字段 | 寄存器 |
|---|---|---|---|---|---|
| `WOPR_BLENDING_ENABLE` | U1.0 | 1 | WOPR 使能（`avg_wopr` 回读前提） | `sre_wopr.blending_enable` | `LTM2_CONFIG.WOPR_BLENDING` |
| `WOPR_THR` | U8.0 | 230 | 亮度阈值（判定"白底"的门限） | `sre_wopr.wopr_thr` | `LTM2_WOPR_THR` |
| `SD_BLENDING_ENABLE` | U1.0 | 1 | scan detection 使能（SRE / FOSS） | `ltm_sd.blending_enable` | `LTM2_CONFIG.SD_BLENDING` |
| `SD_R_THR` / `SD_G_THR` / `SD_B_THR` | U8.0 | 95 / 40 / 20 | R / G / B 通道阈值 | `ltm_sd.r_thr` / `g_thr` / `b_thr` | `LTM2_SD_BLEND_THR_A.RTHR/GTHR/BTHR` |
| `SD_DELTA_MINMAX_THR` | U8.0 | 15 | `delta_rgb`（max-min）阈值 | `ltm_sd.delta_minmax_thr` | `LTM2_SD_BLEND_THR_B.DELTA_MINMAX_THR` |
| `SD_DELTA_RG_THR` | U8.0 | 15 | `abs(R-G)` 阈值 | `ltm_sd.delta_rg_thr` | `LTM2_SD_BLEND_THR_B.DELTA_RG_THR` |

### C.8 FOSS 专用

| 参数 | 类型 | 默认 | 含义 | 字段 | 寄存器 |
|---|---|---|---|---|---|
| `AD_TXT_JUD_EN` | U1.0 | 1 | ROI 文本/区域检测使能（`sum_txt` 回读前提） | `ad_txt_jud_en` | `LTM2_CONFIG.AD_TXT_JUD` |

### C.9 输出级：Tone Adjust 与 Dither

| 参数 | 类型 | 默认 | 含义 | 字段 | 寄存器 |
|---|---|---|---|---|---|
| `ENABLE_TONE_ADJUST` | U1.0 | 1（三种 mode 实际都填 0） | 全局 tone adjust 使能 | `tone_adj.enable` | `LTM2_CONFIG.TONE_ADJUST` |
| `TONE_ADJUST_LUT` | 129 × U11.0 | （算） | 全局 tone adjust LUT。两项拼一个 32bit → 65 次写 | `tone_adj.data[129]`，`entry_cnt` 必须 = 129 | `LTM2_TONE_ADJUST_LUT.VALUE0/VALUE1`（同地址自增） |
| `ENABLE_DITHER` | U1.0 | 1 | dither 使能 | `ltm_dither.dither_enable` | `LTM2_CONFIG.DITHER` |
| `DITHER_TABLE_R[4]` | 4 × U2.0 | 0, 2, 3, 1 | R 通道 2×2 dither 阈值表 | `table_low[0]` / `table_high[0]` | `LTM2_DITHER_TABLE.R00/R01/R10/R11` |
| `DITHER_TABLE_G[4]` | 4 × U2.0 | 1, 0, 3, 2 | G 通道 | `table_low[1]` / `table_high[1]` | `LTM2_DITHER_TABLE.G00/G01/G10/G11` |
| `DITHER_TABLE_B[4]` | 4 × U2.0 | 3, 2, 1, 0 | B 通道 | `table_low[2]` / `table_high[2]` | `LTM2_DITHER_TABLE.B00/B01/B10/B11` |

> **dither 表的打包方式容易配错**：`drm_vs_ltm_dither` 只有 `table_low[3]` /
> `table_high[3]`（按 R/G/B 索引），每个 u32 的低 / 高 16bit 各放一个阈值，
> 驱动取 `& 0x3` 填进 4 个 2bit 字段：
> `R00 = table_low[0] & 3`、`R01 = (table_low[0] >> 16) & 3`、
> `R10 = table_high[0] & 3`、`R11 = (table_high[0] >> 16) & 3`。

### C.10 LTM2.0 **不使用**的结构体字段

存在于 UAPI（LTM1.0 遗留），但 `ltm2_hw_config()` **完全不读** —— 填了也不会写寄存器，
排查时别在这些字段上浪费时间。

| 字段 | LTM1.0 用途 | LTM2.0 替代 |
|---|---|---|
| `ltm_degamma` / `ltm_gamma` | LTM 前后的 de/gamma LUT（65 项） | 不使用 |
| `luma_adj` | guide curve LUT（129 项） | 不使用 |
| `af_filter` | affine slope/bias（各 1152 项）+ 硬件 IIR 权重 | 改用 `ltm_af_luts`（1088 项）；property 被 check 拒绝 |
| `af_trans` | affine 输出 scale LUT（17 项）+ 定点位宽 | 不使用 |
| `ltm_color` | alpha gain / luma / saturation LUT（121/129/257 项） | 改用 `color_protect_enable` 一个开关 |
| `ltm_luma_set` | luma 平均的 margin / pixel_norm | 不使用（`avg_luma` 直接回读） |
| `ltm_cd_set.filt_norm` / `.overlap` | CD 滤波归一化 / overlap | 不使用 |
| `ltm_hist_set.grid_depth` / `.start_pos[]` | 直方图 bin 数 / 起始位置 | 前者移到 `grid_size.hist_depth` |
| `ltm_ds.crop_l/r/t/b` | 下采样 crop | 改用 ROI |
| `ltm_hist_get.result[9216]` | 直方图结果（36KB） | 改用 WDMA 写用户态 BO，只用 `fd` / `hist_bo_handle` |
| `freq_decomp.coef[6..8]` / `.norm` | 3×3 滤波 + 归一化 | 只用 `coef[0..5]` |

---

## 附：参考资料

| 文件 | 内容 |
|---|---|
| `ltm_v2.0/A209_LTM V2.0.Design.Specifications0.7revision.pdf` | 硬件设计规格（本文未引用其硬件原理部分） |
| `ltm_v2.0/ltm2.0_config.docx` | LTM / SRE / FOSS 功能定位、sw 如何调用 algo |
| `ltm_v2.0/寄存器配置与用户参数.xlsx` | 寄存器名/类型/默认值/描述、三模式使能矩阵、APB 回读参数矩阵、用户参数范围与建议值 |
