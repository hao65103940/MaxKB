# coding=utf-8
import os
import gc
import threading
from maxkb.const import CONFIG
from common.utils.logger import maxkb_logger
import tracemalloc

CURRENT_PID=os.getpid()
GC_THRESHOLD = (100, 5, 5)
# 1 hour
GC_INTERVAL = 3600

def change_gc_threshold():
    old_threshold = gc.get_threshold()
    gc.set_threshold(*GC_THRESHOLD)
    maxkb_logger.debug(f"(PID: {CURRENT_PID}) GC thresholds changed from {old_threshold} → {GC_THRESHOLD}")


def force_gc():
    snapshot = tracemalloc.take_snapshot()
    top_stats = snapshot.statistics('lineno')
    maxkb_logger.debug("[ Top 10 memory-consuming lines ]")
    for stat in top_stats[:10]:
        maxkb_logger.debug(stat)
    collected = gc.collect()
    maxkb_logger.debug(f"(PID: {CURRENT_PID}) Forced GC ({collected} objects collected)")
    threading.Timer(GC_INTERVAL, force_gc).start()


def init_memory_optimization():
    tracemalloc.start()
    change_gc_threshold()
    force_gc()
    maxkb_logger.debug("(PID: {CURRENT_PID}) Memory optimization (GC tuning) started.")

if CONFIG.get("ENABLE_MEMORY_OPTIMIZATION", '1') == "1":
    init_memory_optimization()
