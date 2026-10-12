---
name: callflow-mindmap
description: 梳理跨层调用链（用户态→libdrm→内核核心→驱动→硬件），生成思维导图友好的 markdown 文档，可选生成 FlowUs 页面或 XMind 导图。当用户要求"整理/梳理调用流程、调用关系"、"生成调用链/流程文档"、"转成思维导图"时使用。
---

# 跨层调用链梳理 → 思维导图

把"某个对象/功能的完整生命周期调用流"梳理成 md 文档（思维导图友好格式），并按需生成思维导图。
成熟案例：framebuffer 从 drmtest 到 kernel 的生命周期（本仓库 framebuffer_lifecycle.md）。

## 适用场景

- 用户要求梳理某个对象/功能的完整调用链、生命周期（创建/分配/使用/销毁）
- 输出物是 md 文件，后续要在另一个 agent 或工具里转成思维导图
- 跨多个层：用户态测试程序 → 封装库 → 内核核心 → 驱动 → 硬件

## 工作流程

### 第一步：确定范围与输出格式

1. 确认梳理对象、涉及的层和目录（如 drm-test/、libdrm/、linux/drm/、driver/）。
2. 确认输出格式：
   - **思维导图树状 md**（默认）：`#`/`##`/`###` 标题 + 深层嵌套 `-` 列表，每行一个函数，便于转导图。格式规范见 [references/md-tree-format.md](references/md-tree-format.md)。
   - **详细版 md**：加表格、ASCII 总览图（适合直接阅读，不适合转导图）。
3. 确认是否同时生成思维导图（FlowUs / XMind），见 [references/flowus-mindmap.md](references/flowus-mindmap.md)。

### 第二步：并行分层勘察

按层派出多个 Explore agent（一次消息内并行），每层一个：

- 用户态程序层（测试框架、辅助库）
- 封装库/API 层（如 libdrm）
- 内核核心层（如 linux/drm/）
- 驱动层（如 driver/verisilicon/）
- 芯片差异层（可并入驱动层 agent）

每个 agent 的 prompt 必须要求：

- 每项给出：**函数名、文件（仓库相对路径）、行号（用 Grep -n 确认）、谁调用谁**
- 按生命周期阶段组织报告（创建→分配→使用→销毁 之类）
- 只报告真实存在的代码，不臆测；明确列出"提问假设中不存在"的函数名
- 标注与假设不符之处（如内核版本布局差异、某个封装函数不存在）

agent 运行期间不要重复它们的搜索。

### 第三步：核实关键锚点

写文档前，用 Grep 亲自抽查最关键的锚点（用户态入口函数、ioctl 分发表、驱动回调注册点、核心处理函数），发现偏差立即修正。关键锚点示例：

- ioctl 分发表：`grep -n "DRM_IOCTL_DEF(DRM_IOCTL_MODE_" linux/drm/drm_ioctl.c`
- 驱动 fops 注册（unlocked_ioctl/mmap 指向谁）
- 用户态入口（bo_create_dumb 等）

### 第四步：确定阶段顺序（重要）

**阶段顺序必须按代码的实际执行顺序**，不能想当然。查看用户态调用点的实际先后：

- 例：`atomic_set_plane` 里先 `vs_bo_create`（CREATE_DUMB 拿 handle）**再** `drmModeAddFB2WithModifiers`（用 handle 注册 FB）→ 所以阶段顺序是"分配→注册"而不是"创建→分配"。这类顺序错误用户会纠正，主动查证可避免返工。

### 第五步：写思维导图树状 md

按 [references/md-tree-format.md](references/md-tree-format.md) 的格式写，必须体现：

1. **用户态→内核态衔接显式化**（用户重点关注）：
   - ioctl 通道：`用户态函数 → drmIoctl → ioctl(2) 系统调用 → fops.unlocked_ioctl → 分发表查表 → 内核 handler`，每级标注 `(文件:行号)` 或 `文件.c`
   - 例外通道：mmap 不走 ioctl，直接进 fops 的 .mmap 回调
2. **回调机制**：core → 驱动通过函数指针表回调，标注 `--xxx 回调`，写清注册处和指向
3. **层切换标注**：函数行尾附所在 .c 文件（如 `vs_gem_dumb_create( ) vs_gem.c --driver->dumb_create 回调`）
4. **关键点展开更深**：用户关注的点（如 GEM 分配链）展开到子函数级，每层带一行注释
5. 每个函数一行，注释简短（`--` 开头，说明作用/关键动作）
6. **关键函数加中文多行注释**：核心环节（如 GEM 分配、fb 注册、atomic 提交）的函数下挂 2-3 行 `- --` 中文注释子行，解释"做什么 + 为什么/关键分支"，格式见 [references/md-tree-format.md](references/md-tree-format.md)

### 第六步：生成思维导图（可选）

见 [references/flowus-mindmap.md](references/flowus-mindmap.md)：FlowUs API 写入嵌套块，或生成 XMind/FreeMind 文件。

### 第七步：按反馈迭代

常见反馈及应对：

- "阶段顺序有问题" → 回到第四步查实际执行顺序
- "调用关系写详细点" → 把关注链路展开到子函数级
- "总览图详细点" → 用 ASCII 纵向链 + 层级标签（[用户态]/[内核态·入口]/[内核态·DRM核心]/[内核态·驱动]）
- "改成某某格式" → 读用户的参考文件，严格对齐其格式

## 关键经验

1. 行号必须核实：agent 报告 + 自己 Grep 抽查双重验证，文档里的链接错误会误导后续所有使用者。
2. 记录"不存在项"：如 libdrm 没有 drmModeDumbCreate 封装、驱动没有自定义 fb 销毁回调——避免后续臆测 API 名。
3. 用户态唯一进内核的通道是 ioctl 和 mmap，其余全部是进程内普通函数调用；跨界点要在树里显式标出。
4. 输出文件命名与内容匹配（如 framebuffer_lifecycle.md），用户可能要求重命名时主动取贴合内容的名字。
