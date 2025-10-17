<template>
<div>
    <span>变量聚合</span>
</div>
</template>
<script setup lang="ts">
import { set, groupBy, cloneDeep } from 'lodash'
import NodeCascader from '@/workflow/common/NodeCascader.vue'
import NodeContainer from '@/workflow/common/NodeContainer.vue'
import AIModeParamSettingDialog from '@/views/application/component/AIModeParamSettingDialog.vue'
import type { FormInstance } from 'element-plus'
import { ref, computed, onMounted, inject } from 'vue'
import { isLastNode } from '@/workflow/common/data'
import { t } from '@/locales'
import { useRoute } from 'vue-router'
import { randomId } from '@/utils/common'
import { loadSharedApi } from '@/utils/dynamics-api/shared-api'

</script>
<style lang="scss" scoped></style>
