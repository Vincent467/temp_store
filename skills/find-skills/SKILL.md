---
name: find-skills
description: 帮助用户发现并安装 agent 技能。当用户提出"怎么做 X"、"我找个能做 X 的技能"、"有没有能 X 的技能"之类的问题，或表示希望扩展能力时使用。当用户正在寻找可能以可安装技能形式存在的能力时，应使用本技能。
---

# 寻找技能 (Find Skills)

本技能帮助用户从开源 agent 技能生态中发现并安装技能。

## 何时使用本技能

当用户出现以下情况时使用：

- 问"怎么做 X"，而 X 可能是已有技能覆盖的常见任务
- 说"找一个做 X 的技能"或"有没有做 X 的技能"
- 问"你能不能做 X"，且 X 是某种专业能力
- 表示希望扩展 agent 能力
- 想搜索工具、模板或工作流
- 提到希望某个特定领域（设计、测试、部署等）的帮助

## Skills CLI 是什么？

Skills CLI（`npx skills`）是开源 agent 技能生态的包管理器。技能是可扩展 agent 能力（专业知识、工作流和工具）的模块化包。

**关键命令：**

- `npx skills find [查询词] [--owner <owner>]` - 交互式或按关键字搜索技能，可选限定 GitHub owner 范围
- `npx skills add <包名>` - 从 GitHub 或其他来源安装技能
- `npx skills update` - 更新所有已安装的技能

**技能浏览网站：** https://skills.sh/

## 如何帮助用户寻找技能

### 第一步：理解用户需求

当用户请求帮助时，识别：

1. 领域（如 React、测试、设计、部署）
2. 具体任务（如编写测试、创建动画、审查 PR）
3. 该任务是否足够常见以至于很可能已有现成技能

### 第二步：先看排行榜

运行 CLI 搜索之前，先查看 [skills.sh 排行榜](https://skills.sh/)，确认该领域是否已有知名技能。排行榜按总安装量排序，展示最受欢迎、经过实战检验的选项。

例如，Web 开发领域的头部技能包括：
- `vercel-labs/agent-skills` — React、Next.js、网页设计（各 10 万+ 安装）
- `anthropics/skills` — 前端设计、文档处理（各 10 万+ 安装）

### 第三步：搜索技能

如果排行榜没有覆盖用户需求，运行 find 命令：

```bash
npx skills find [查询词] [--owner <owner>]
```

例如：

- 用户问"怎么让我的 React 应用更快？" → `npx skills find react performance`
- 用户问"能帮我做 PR 审查吗？" → `npx skills find pr review`
- 用户问"我需要创建变更日志" → `npx skills find changelog`

### 第四步：推荐前先验证质量

**不要仅凭搜索结果就推荐技能。** 务必验证：

1. **安装量** — 优先选择 1K+ 安装的技能。低于 100 的需谨慎对待。
2. **来源信誉** — 官方来源（`vercel-labs`、`anthropics`、`microsoft`）比不知名作者更可信。
3. **GitHub 星标** — 查看源仓库。来自星标数 <100 仓库的技能应保持怀疑。

### 第五步：向用户呈现选项

找到相关技能后，向用户展示：

1. 技能名称及功能
2. 安装量和来源
3. 用户可执行的安装命令
4. 在 skills.sh 了解更多信息的链接

示例回复：

```
我找到了可能对你有帮助的技能！"react-best-practices" 技能提供来自
Vercel Engineering 的 React 和 Next.js 性能优化指南。
（18.5 万安装）

安装命令：
npx skills add vercel-labs/agent-skills@react-best-practices

了解更多：https://skills.sh/vercel-labs/agent-skills/react-best-practices
```

### 第六步：主动提供安装

如果用户想继续，你可以帮其安装：

```bash
npx skills add <owner/repo@skill> -g -y
```

`-g` 表示全局安装（用户级），`-y` 跳过确认提示。

## 常见技能类型

搜索时可考虑这些常见类别：

| 类别            | 示例查询词                                    |
| --------------- | --------------------------------------------- |
| Web 开发        | react, nextjs, typescript, css, tailwind      |
| 测试            | testing, jest, playwright, e2e                |
| DevOps          | deploy, docker, kubernetes, ci-cd             |
| 文档            | docs, readme, changelog, api-docs             |
| 代码质量        | review, lint, refactor, best-practices        |
| 设计            | ui, ux, design-system, accessibility          |
| 生产力          | workflow, automation, git                     |

## 高效搜索技巧

1. **使用具体关键词**："react testing" 比只搜 "testing" 更好
2. **尝试替代词**：如果 "deploy" 没结果，试试 "deployment" 或 "ci-cd"
3. **关注热门来源**：许多技能来自 `vercel-labs/agent-skills` 或 `ComposioHQ/awesome-claude-skills`

## 找不到技能时

如果没有相关技能：

1. 说明未找到现有技能
2. 提议直接用你的通用能力帮助用户完成任务
3. 建议用户用 `npx skills init` 创建自己的技能

示例：

```
我搜索了与 "xyz" 相关的技能，但没有找到匹配项。
我仍然可以直接帮你完成这个任务！要开始吗？

如果这是你经常做的事，可以创建自己的技能：
npx skills init my-xyz-skill
```
