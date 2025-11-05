"""
WSGI config for maxkb project.

It exposes the WSGI callable as a module-level variable named ``application``.

For more information on this file, see
https://docs.djangoproject.com/en/4.2/howto/deployment/wsgi/
"""

import os

from django.core.wsgi import get_wsgi_application
#
# # 检查是否启用 memray 分析
# if os.environ.get('ENABLE_MEMRAY') == '1':
#     import memray
#     import atexit
#
#     # 为每个进程创建单独的追踪文件
#     pid = os.getpid()
#     output_file = f"memray_output_{pid}.bin"
#
#     tracker = memray.Tracker(output_file)
#     tracker.__enter__()
#
#
#     def cleanup():
#         tracker.__exit__(None, None, None)
#
#
#     atexit.register(cleanup)

os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'maxkb.settings')

application = get_wsgi_application()


# -----------------------------
# 全局初始化，只希望在 master preload 阶段执行一次
# -----------------------------
def preloaded_init():
    from common.database_model_manage.database_model_manage import DatabaseModelManage
    from common import event
    from common.utils.logger import maxkb_logger

    event.run()
    DatabaseModelManage.init()

    if os.environ.get("ENABLE_SCHEDULER") == "1":
        from common import job

        job.run()

    maxkb_logger.info("✅ preloaded_init: master 初始化完成，内存将被 worker 共享")


# Gunicorn preload 阶段会执行此逻辑
if os.environ.get("GUNICORN_PRELOAD", "1") == "1":
    try:
        preloaded_init()
    except Exception as e:
        import traceback
        from common.utils.logger import maxkb_logger

        maxkb_logger.info("⚠️ preload 初始化失败:", e)
        traceback.print_exc()
