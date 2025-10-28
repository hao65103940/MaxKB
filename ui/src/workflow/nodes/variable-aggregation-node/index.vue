<template>
  <NodeContainer :nodeModel="nodeModel">
    <h5 class="title-decoration-1 mb-8">{{ $t('views.applicationWorkflow.nodeSetting') }}</h5>
    <el-form
        @submit.prevent
        :model="form_data"
        label-position="top"
        require-asterisk-position="right"
        label-width="auto"
        ref="VariableAggregationRef"
        hide-required-asterisk
      >
      <el-form-item
          :label="$t('views.applicationWorkflow.nodes.variableAggregationNode.Strategy')"
          :rules="{
            required: true,
            trigger: 'change',
          }"
        >
          <template #label>
            <div class="flex-between">
              <div>
                <span>{{ $t('views.applicationWorkflow.nodes.variableAggregationNode.Strategy') }}
                  <span class="color-danger">*</span>
                </span>
              </div>
            </div>
          </template>
          <el-select
            v-model="form_data.strategy"
          >
            <el-option
              :label="t('views.applicationWorkflow.nodes.variableAggregationNode.placeholder')"
              value="first_non_null"
            />
            <el-option
              :label="t('views.applicationWorkflow.nodes.variableAggregationNode.placeholder1')"
              value="variable_to_json"
            />
          </el-select>
      </el-form-item>
      <div v-for="(group, gIndex) in form_data.group_list" :key="group.id" class="mb-8">
        <el-card shadow="never" class="card-never" style="--el-card-padding: 12px">
          
          <div class="flex-between mb-12">
            <span class="font-bold">{{ group.label }}</span>
            <div class="flex align-center">
              <el-button @click="openAddOrEditDialog(group,gIndex)" size="large" link>
                <el-icon><EditPen /></el-icon>
              </el-button>
              <el-button @click="deleteGroup(gIndex)" size="large" link :disabled="form_data.group_list.length <= 1">
                <AppIcon iconName="app-delete"></AppIcon>
              </el-button>
            </div>
          </div>

          <div v-for="(item, vIndex) in group.variable_list" :key="item.v_id" class="mb-4">
            <el-row :gutter="8">
              <el-col :span="21">
                <el-form-item
                  :prop="`group_list.${gIndex}.variable_list.${vIndex}.variable`"
                  :rules="{
                    type: 'array',
                    required: true,
                    message: $t('views.applicationWorkflow.nodes.variableAggregationNode.group.placeholder'),
                    trigger: 'change',
                  }"
                >
                  <NodeCascader
                    ref="nodeCascaderRef"
                    :nodeModel="nodeModel"
                    class="w-full"
                    :placeholder="$t('views.applicationWorkflow.nodes.variableAggregationNode.group.placeholder')"
                    v-model="item.variable"
                  />
                </el-form-item>
              </el-col>
              <el-col :span="3" style="text-align: center;">
                <el-button
                  link
                  size="large"
                  class="mt-4"
                  :disabled="group.variable_list.length <= 1"
                  @click="deleteVariable(gIndex, vIndex)"
                >
                  <AppIcon iconName="app-delete"></AppIcon>
                </el-button>
              </el-col>
            </el-row>
          </div>

          <el-button @click="addVariable(gIndex)" type="primary" size="large" link>
            <AppIcon iconName="app-add-outlined" class="mr-4"/>
            {{ $t('common.add') }}
          </el-button>
          
        </el-card>
      </div>
      <el-button @click="openAddOrEditDialog()" type="primary" size="large" link>
        <AppIcon iconName="app-add-outlined" class="mr-4"/>
            {{ $t('views.applicationWorkflow.nodes.variableAggregationNode.add') }}
      </el-button>
      </el-form>
      <GroupFieldDialog ref="GroupFieldDialogRef" @refresh="refreshFieldList"></GroupFieldDialog>
  </NodeContainer>
</template>
<script setup lang="ts">
import { set, cloneDeep } from 'lodash'
import NodeCascader from '@/workflow/common/NodeCascader.vue'
import NodeContainer from '@/workflow/common/NodeContainer.vue'
import GroupFieldDialog from './component/GroupFieldDialog.vue'
import { ref, computed, onMounted } from 'vue'
import { isLastNode } from '@/workflow/common/data'
import { t } from '@/locales'
import { randomId } from '@/utils/common'
import { MsgError } from '@/utils/message'

const props = defineProps<{ nodeModel: any }>()
const VariableAggregationRef = ref()
const nodeCascaderRef = ref()
const GroupFieldDialogRef = ref()

const form = {
  strategy: 'first_non_null',
  group_list: [
    {
      id: randomId(),
      label: 'Group1',
      field: 'Group1',
      variable_list: [
        {
          v_id: randomId(),
          variable: []
        }
      ]
    },
  ]
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
  }
})

const inputFieldList = ref<any[]>([])

function openAddOrEditDialog(group?: any, index?: any) {
  let data = null
  if (group && index !== undefined) {
    data = {
      field: group.field,
      label: group.label,
    }
  }
  GroupFieldDialogRef.value.open(data,index)
}

function refreshFieldList(data: any, index: any) {
  for (let i = 0; i < inputFieldList.value.length; i++) {
    if (inputFieldList.value[i].field === data.field && index !== i) {
      MsgError(t('views.applicationWorkflow.tip.paramErrorMessage') + data.field)
      return
    }
  }
  if ([undefined, null].includes(index)) {
    inputFieldList.value.push(data)
    addGroup(data)
  } else {
    inputFieldList.value.splice(index, 1, data)
    editGroupDesc(data, index)
  }
  GroupFieldDialogRef.value.close()
  const fields = [
    ...inputFieldList.value.map((item) => ({ label: item.label, value: item.field })),
  ]
  set(props.nodeModel.properties.config, 'fields', fields)
}

const editGroupDesc = (data: any, gIndex: any) => {
  const c_group_list = cloneDeep(form_data.value.group_list)
  c_group_list[gIndex].field = data.field
  c_group_list[gIndex].label = data.label
  form_data.value.group_list = c_group_list
}

const deleteGroup = (gIndex: number) => {
  const c_group_list = cloneDeep(form_data.value.group_list)
  c_group_list.splice(gIndex,1)
  form_data.value.group_list = c_group_list
  inputFieldList.value.splice(gIndex, 1)
  const fields = c_group_list.map((item:any) => ({ label: item.label, value: item.field}))
  set(props.nodeModel.properties.config, 'fields', fields)
}

const addVariable = (gIndex: number) => {
  const c_group_list = cloneDeep(form_data.value.group_list)
  c_group_list[gIndex].variable_list.push({
      v_id: randomId(),
          variable: []
  })
  form_data.value.group_list = c_group_list
}

const deleteVariable = (gIndex: number,vIndex: number) => {
  const c_group_list = cloneDeep(form_data.value.group_list)
  c_group_list[gIndex].variable_list.splice(vIndex, 1)
  form_data.value.group_list = c_group_list
}

const addGroup = (data: any) => {
  const c_group_list = cloneDeep(form_data.value.group_list)
  c_group_list.push({
    id: randomId(),
    field: data.field,
    label: data.label,  
      variable_list: [{
          v_id: randomId(),
          variable: []
        }]
  })
  form_data.value.group_list = c_group_list
}



const validate = async () => {
  const validate_list = [
    ...nodeCascaderRef.value.map((item:any)=>item.validate()),
    VariableAggregationRef.value?.validate(),
  ]
  return Promise.all(validate_list).catch((err) => {
    return Promise.reject({node: props.nodeModel, errMessage: err})
  })
}

onMounted(() => {
  if (typeof props.nodeModel.properties.node_data?.is_result === 'undefined') {
    if (isLastNode(props.nodeModel)) {
      set(props.nodeModel.properties.node_data, 'is_result', true)
    }
  }
  set(props.nodeModel, 'validate', validate)
  if (props.nodeModel.properties.node_data.group_list) {
    inputFieldList.value = form_data.value.group_list.map((item:any) => ({ label: item.label, field: item.field}))
  }
  const fields = form_data.value.group_list.map((item: any) => ({ label: item.label, value: item.field }))
  set(props.nodeModel.properties.config, 'fields', fields)
})


</script>
<style lang="scss" scoped></style>
