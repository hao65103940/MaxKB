# coding=utf-8
import os
import gc
import threading
from maxkb.const import CONFIG
from common.utils.logger import maxkb_logger
import random

CURRENT_PID=os.getpid()
# 1 hour
GC_INTERVAL = 3600

def force_gc():
    collected = gc.collect()
    maxkb_logger.debug(f"(PID: {CURRENT_PID}) Forced GC ({collected} objects collected)")
    threading.Timer(GC_INTERVAL - random.randint(0, 900), force_gc).start()

if CONFIG.get("ENABLE_MEMORY_OPTIMIZATION", '1') == "1":
    threading.Timer(GC_INTERVAL - random.randint(0, 900), force_gc).start()
