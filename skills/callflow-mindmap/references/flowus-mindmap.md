# 思维导图生成：FlowUs / XMind

## 现状结论（2026-10 查证）

- FlowUs 有官方 V2 Open API：`https://api.flowus.cn`，路径前缀 `/v2`，认证 `Authorization: Bearer <token>`（token 在 FlowUs 设置 → 集成 中创建）。
- **思维导图在 API 中是页面类型**（`page_type: "mind_map"`），不是 block 类型；API 无"向思维导图写节点"的接口，用 API 创建 mind_map 页面只能得到空导图页。
- API 支持的 block 类型：paragraph、heading_1/2/3、bulleted_list_item、numbered_list_item、to_do、callout、divider、table 等——**没有 mind_map block**。
- 我无法登录用户账号操作 FlowUs 网页界面（WebFetch 对登录后页面无效，无浏览器自动化），只能走 API 或生成文件。

## 方案 A：FlowUs API 写嵌套结构（可自动化）

1. 用户提供 API token（需 pages/blocks 写权限 scope）+ 目标空间 ID。
2. 解析 md 树：`#`→页面标题或 heading_1，`##`→heading_2，`###`→heading_3，`-` 嵌套 → bulleted_list_item 按缩进层级挂父子关系（API 一次最多追加一定数量子块，需分批 append）。
3. 效果：FlowUs 里得到完整树状大纲（视觉即树形），用户可在界面内一键转思维导图。
4. 注意：向用户说清"无法直接生成可视导图块"的限制，让其选择方案。

## 方案 B：生成 XMind / FreeMind 文件（推荐给想直接得到导图的用户）

- **XMind**：.xmind 是 zip 包，内含 content.json（主题树：rootTopic → children[].topics），生成后用户用 XMind 打开或 FlowUs 文件块上传。
- **FreeMind**：.mm 是 XML（`<map><node TEXT="...">` 嵌套），XMind/FreeMind 均可导入。
- 做法：把 md 树解析为 JSON 树（标题层级 + 列表缩进 → 父子关系），再序列化为目标格式。

## 执行前提清单（开始前向用户确认）

- [ ] 选方案 A / B / 都要
- [ ] 方案 A：API token + 空间 ID（或授权后调 API 查询）
- [ ] 目标文档名/存放位置
- [ ] 节点内容用中文原文（是，默认）

## 参考链接

- FlowUs API 概览：https://flowus.cn/developer-api/v2/zh/getting-started/overview
- 对象模型（page_type 含 mind_map）：https://flowus.cn/developer-api/v2/zh/reference/object-models
- SDK（支持的 block 类型）：https://github.com/next-space/flowus-api-sdk
