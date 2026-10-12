# 思维导图树状 md 格式规范

## 结构

```
# 入口函数( )                     ← 一级：程序入口（如 main）
## 阶段一函数( ) 阶段说明           ← 二级：生命周期阶段/顶层流程
### 子步骤( )                      ← 三级：阶段内子步骤
- 函数( ) 文件.c --注释            ← 四级起：调用链，用嵌套 - 逐级展开
  - 被调用函数( ) 文件.c --注释
    - 更深层( ) --每层缩进两个空格
```

## 每行格式

`- 函数名( ) 所在.c --一行注释`

- 函数名后固定 `( )`
- 文件后缀 `.c`（或 .h）直接附在函数名后，用于标注层切换；同文件可省略
- 注释以 `--` 开头，说明该步作用/关键动作，一行内写完
- 跨界点、回调点用注释标注：
  - `--ioctl 系统调用，进入内核`
  - `--driver->dumb_create 回调`
  - `--fops.unlocked_ioctl，查 drm_ioctls[] 分发表`
  - `--纯用户态数组累积，无 ioctl`

## 关键函数注释（中文）

关键函数（GEM 分配、fb 注册、atomic 提交等核心环节）除了行尾一句话注释，还可以挂 **2-3 行中文说明**，写成函数下的子级 `- --` 行（缩进一层），保持树结构可转思维导图：

```markdown
- vs_gem_alloc_buf( ) --物理内存分配
  - --dma_alloc_attrs 分配连续 dma 缓存，得到 dma_addr/cookie
  - --MMU 开启时经 dc_mmu_map_memory_and_flush 映射得到 iova；无 MMU 时 iova = dma_addr
  - --无 MMU 非连续后备：alloc_pages + SetPageReserved
```

要点：
- 注释必须中文、简短（每行一句话，总共 2-3 行），解释"做什么 + 为什么/关键分支"，不写废话
- 只给关键函数加多行注释；一般函数保持行尾一句话即可
- `- --` 子行是注释节点，转导图后挂在函数节点下，不影响主干结构

## 必须体现的要素

1. **ioctl 通道完整链**（每个 ioctl 都要写全）：
   `用户态函数 → drmIoctl(XXX) → ioctl(2) 边界 → drm_ioctl( ) 查表 → 内核 handler → 往下`
2. **mmap 例外**：注明"不走 ioctl，直接进 fops 的 .mmap 回调"
3. **回调点**：core → 驱动每次经过函数指针表都标注 `--xxx 回调`，并注明注册处文件
4. **阶段顺序**：按实际执行顺序排（先分配后注册等）

## 示例（framebuffer_lifecycle.md 片段）

```markdown
# main( )
## dpu_set_frame( ) 单帧提交：先分配 BO 拿 handle，再注册 FB，最后 atomic commit
### atomic_set_planes( )
- atomic_set_plane( ) --每 plane 一条调用
  - vs_bo_create( ) buffers.c --创建 bo：内核分配 GEM，用户态映射并填充数据
    - bo_create_dumb( ) --填 struct drm_mode_create_dumb（bpp/width/height，pitch 对齐写入 arg.flags）
      - drmIoctl(DRM_IOCTL_MODE_CREATE_DUMB)( ) xf86drm.c --ioctl 系统调用，进入内核
        - drm_ioctl( ) drm_ioctl.c --fops.unlocked_ioctl，查 drm_ioctls[] 分发表
          - drm_mode_create_dumb_ioctl( ) drm_dumb_buffers.c --校验 bpp/宽高，清零输出 handle/pitch/size
            - vs_gem_dumb_create( ) vs_gem.c --driver->dumb_create 回调
              - pitch 计算 --DIV_ROUND_UP(width*bpp,8) 后按 priv->pitch_alignment 对齐
              - args->size = PAGE_ALIGN(pitch*height) --整页对齐的大小
              - vs_gem_create_with_handle( )
                - vs_gem_alloc_buf( ) --物理内存分配
                  - dma_alloc_attrs( ) --分配连续 dma 缓存，得 dma_addr/cookie
                - drm_gem_handle_create( ) drm_gem.c --发布 handle 返回用户态
  - drmModeAddFB2WithModifiers( ) --用 GEM handle 注册 framebuffer，得到 fb_id
    - drmIoctl(DRM_IOCTL_MODE_ADDFB2)( ) xf86drmMode.c
      - drm_ioctl( ) drm_ioctl.c --查 drm_ioctls[] 分发表
        - drm_mode_addfb2_ioctl( ) drm_framebuffer.c
          - drm_mode_addfb2( ) --检查 DRIVER_MODESET，fb 挂入 file_priv->fbs 链表
            - vs_fb_create( ) vs_fb.c --mode_config.funcs->fb_create 回调
```

## 深度原则

- 默认：链到"层切换点 + 核心动作"即可
- 用户点名的关注点（如 GEM 分配）：展开到子函数级，每层带注释
- 关键动作可写成伪代码行：`- plane->dma_addr[i] = vs_obj->iova + fb->offsets[i] --核心一步`
