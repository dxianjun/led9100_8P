# led9100 GitHub版本管理规范

## 1. 目标
- 保证代码可追溯、可回滚、可发布。
- 控制提交粒度，降低合并风险。
- 让每个版本都有明确编号和变更说明。

## 2. 分支策略
- `main`：可发布分支，只放可编译、可交付代码。
- `feature/*`：功能开发分支，例如 `feature/bt-wakeup`。
- `fix/*`：问题修复分支，例如 `fix/uart-parse`。
- `release/*`（可选）：版本冻结/联调分支，例如 `release/v4.1.0`。

## 3. 日常开发流程
1. 从 `main` 拉最新：
   - `git switch main`
   - `git pull origin main`
2. 创建任务分支：
   - `git switch -c feature/xxx`
3. 开发并小步提交：
   - `git add -A`
   - `git commit -m "feat(bt): add wakeup pulse api"`
4. 推送远端：
   - `git push -u origin feature/xxx`
5. 自测通过后合并到 `main`（可用PR或本地合并）。

## 4. 提交信息规范
- 推荐格式：`type(scope): summary`
- 常用 `type`：
  - `feat`：新功能
  - `fix`：缺陷修复
  - `refactor`：重构（不改行为）
  - `docs`：文档更新
  - `chore`：构建/配置/杂项
- 示例：
  - `feat(bt): add bt_wakeup interface on PA2`
  - `fix(protocol): reject invalid cmd with ack 0x10`

## 5. 发布与Tag
1. 在 `main` 确认可编译可运行。
2. 打注释标签：
   - `git tag -a v1.0.1 -m "led9100 v1.0.1"`
3. 推送代码和标签：
   - `git push origin main`
   - `git push origin v1.0.1`

## 6. 回滚策略
- 回退单次提交（推荐，保留历史）：
  - `git revert <commit-id>`
- 回退到某版本查看：
  - `git checkout v4.0.0`
- 禁止直接用破坏性命令覆盖团队历史（如未确认就 `reset --hard`）。

## 7. 嵌入式项目特别约束
- 不提交编译产物（如 `Objects/`, `Listings/`, `.axf`, `.hex`, `.map`）。
- 不提交 IDE 缓存/临时文件（如 Source Insight cache）。
- 硬件脚位变更必须同步文档与代码注释。
- 每次发布前至少执行一次完整编译与基础功能回归。

## 8. 常用命令速查
- 查看状态：`git status -sb`
- 查看最近提交：`git log --oneline -n 10`
- 查看改动详情：`git diff`
- 暂存全部：`git add -A`
- 提交：`git commit -m "msg"`
- 推送当前分支：`git push`
- 拉取更新：`git pull`

## 9. 建议执行节奏
- 每完成一个可验证小功能就提交一次。
- 每天至少 push 一次远端备份。
- 每个可对外交付节点打一个语义化 tag（如 `v4.0.2`）。
