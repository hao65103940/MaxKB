<template>
  <NodeContainer :nodeModel="nodeModel">
    <el-form
      @submit.prevent
      :model="form_data"
      label-position="top"
      require-asterisk-position="right"
      label-width="auto"
      ref="VariableSplittingRef"
      hide-required-asterisk
    >
      <el-form-item
        :label="$t('views.applicationWorkflow.nodes.variableSplittingNode.label', '输入变量')"
      >
        <template #label>
          <div class="flex-between">
            <div>
              {{ $t('views.applicationWorkflow.nodes.variableSplittingNode.label', '输入变量') }}
              <span class="color-danger">*</span>
            </div>
          </div>
        </template>
        <NodeCascader
          ref="nodeCascaderRef"
          :nodeModel="nodeModel"
          class="w-full"
          :placeholder="$t('views.applicationWorkflow.variable.placeholder')"
          v-model="form_data.input_variable"
        />
      </el-form-item>
      <VariableFieldTable ref="VariableFieldTableRef" :node-model="nodeModel"></VariableFieldTable>
    </el-form>
  </NodeContainer>
</template>
<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import NodeContainer from '@/workflow/common/NodeContainer.vue'
import NodeCascader from '@/workflow/common/NodeCascader.vue'
import VariableFieldTable from '@/workflow/nodes/variable-splitting/component/VariableFieldTable.vue'
import { set } from 'lodash'
const props = defineProps<{ nodeModel: any }>()

const form = {
  input_variable: [],
  variable_list: [],
}

const form_data = computed({
  get: () => {
    if (props.nodeModel.properties.node_data) {
      return props.nodeModel.properties.node_data
    } else {
      set(props.nodeModel.properties, 'node_data', form)
    }
    return props.nodeModel.properties.node_data
  },
  set: (value) => {
    set(props.nodeModel.properties, 'node_data', value)
  },
})
const VariableSplittingRef = ref()
const validate = async () => {
  return VariableSplittingRef.value.validate()
}
onMounted(() => {
  set(props.nodeModel, 'validate', validate)
})
</script>
<style lang="scss" scoped></style>
