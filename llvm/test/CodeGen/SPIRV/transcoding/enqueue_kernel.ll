; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'enqueue_kernel.cl'
source_filename = "enqueue_kernel.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV: OpEntryPoint {{.*}} %[[BlockKer1:[0-9]+]] "__device_side_enqueue_block_invoke_kernel"
; CHECK-SPIRV: OpEntryPoint {{.*}} %[[BlockKer2:[0-9]+]] "__device_side_enqueue_block_invoke_2_kernel"
; CHECK-SPIRV: OpEntryPoint {{.*}} %[[BlockKer3:[0-9]+]] "__device_side_enqueue_block_invoke_3_kernel"
; CHECK-SPIRV: OpEntryPoint {{.*}} %[[BlockKer4:[0-9]+]] "__device_side_enqueue_block_invoke_4_kernel"
; CHECK-SPIRV: OpEntryPoint {{.*}} %[[BlockKer5:[0-9]+]] "__device_side_enqueue_block_invoke_5_kernel"
; CHECK-SPIRV: OpName %[[BlockGlb1:[0-9]+]] "__block_literal_global"
; CHECK-SPIRV: OpName %[[BlockGlb2:[0-9]+]] "__block_literal_global.1"

; CHECK-SPIRV: %[[Int32Ty:[0-9]+]] = OpTypeInt 32
; CHECK-SPIRV: %[[Int8Ty:[0-9]+]] = OpTypeInt 8
; CHECK-SPIRV: %[[ConstInt0:[0-9]+]] = OpConstant %[[Int32Ty]] 0
; CHECK-SPIRV: %[[ConstInt17:[0-9]+]] = OpConstant %[[Int32Ty]] 21
; CHECK-SPIRV: %[[ConstInt2:[0-9]+]] = OpConstant %[[Int32Ty]] 2
; CHECK-SPIRV: %[[ConstInt8:[0-9]+]] = OpConstant %[[Int32Ty]] 8
; CHECK-SPIRV: %[[ConstInt20:[0-9]+]] = OpConstant %[[Int32Ty]] 24

; CHECK-SPIRV: %{{[0-9]+}} = OpTypePointer Function %{{[0-9]+}}
; CHECK-SPIRV: %[[Int8PtrGenTy:[0-9]+]] = OpTypePointer Generic %[[Int8Ty]]
; CHECK-SPIRV: %[[VoidTy:[0-9]+]] = OpTypeVoid
; CHECK-SPIRV: %[[Int32LocPtrTy:[0-9]+]] = OpTypePointer Function %[[Int32Ty]]
; CHECK-SPIRV: %[[EventTy:[0-9]+]] = OpTypeDeviceEvent
; CHECK-SPIRV: %[[EventPtrTy:[0-9]+]] = OpTypePointer Generic %[[EventTy]]
; CHECK-SPIRV: %[[BlockTy1:[0-9]+]] = OpTypeFunction %[[VoidTy]] %[[Int8PtrGenTy]]
; CHECK-SPIRV: %[[BlockTy2:[0-9]+]] = OpTypeFunction %[[VoidTy]] %[[Int8PtrGenTy]]
; CHECK-SPIRV: %[[BlockTy3:[0-9]+]] = OpTypeFunction %[[VoidTy]] %[[Int8PtrGenTy]]
; CHECK-SPIRV: %[[EventNull:[0-9]+]] = OpConstantNull %[[EventPtrTy]]

; typedef struct {int a;} ndrange_t;
; #define NULL ((void*)0)

; kernel void device_side_enqueue(global int *a, global int *b, int i, char c0) {
;   queue_t default_queue;
;   unsigned flags = 0;
;   ndrange_t ndrange;
;   clk_event_t clk_event;
;   clk_event_t event_wait_list;
;   clk_event_t event_wait_list2[] = {clk_event};

  ; Emits block literal on stack and block kernel.

  ; CHECK-SPIRV: %[[BlockLit1:[0-9]+]] = OpPtrCastToGeneric %[[Int8PtrGenTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpEnqueueKernel %[[Int32Ty]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ;                            %[[ConstInt0]] %[[EventNull]] %[[EventNull]]
  ;                            %[[BlockKer1]] %[[BlockLit1]] %[[ConstInt17]] %[[ConstInt8]]

;   enqueue_kernel(default_queue, flags, ndrange,
;                  ^(void) {
;                    a[i] = c0;
;                  });

  ; Emits block literal on stack and block kernel.

  ; CHECK-SPIRV: %[[Event1:[0-9]+]] = OpPtrCastToGeneric %[[EventPtrTy]]
  ; CHECK-SPIRV: %[[Event2:[0-9]+]] = OpPtrCastToGeneric %[[EventPtrTy]]

  ; CHECK-SPIRV: %[[BlockLit2:[0-9]+]] = OpPtrCastToGeneric %[[Int8PtrGenTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpEnqueueKernel %[[Int32Ty]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ;                            %[[ConstInt2]] %[[Event1]] %[[Event2]]
  ;                            %[[BlockKer2]] %[[BlockLit2]] %[[ConstInt20]] %[[ConstInt8]]

;   enqueue_kernel(default_queue, flags, ndrange, 2, &event_wait_list, &clk_event,
;                  ^(void) {
;                    a[i] = b[i];
;                  });

;   char c;
  ; Emits global block literal and block kernel.

  ; CHECK-SPIRV: %[[LocalBuf31:[0-9]+]] = OpPtrAccessChain %[[Int32LocPtrTy]]
  ; CHECK-SPIRV: %[[BlockLit3Tmp:[0-9]+]] = OpBitcast %{{[0-9]+}} %[[BlockGlb1:[0-9]+]]
  ; CHECK-SPIRV: %[[BlockLit3:[0-9]+]] = OpPtrCastToGeneric %[[Int8PtrGenTy]] %[[BlockLit3Tmp]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpEnqueueKernel %[[Int32Ty]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ;                            %[[ConstInt2]] %[[Event1]] %[[Event2]]
  ;                            %[[BlockKer3]] %[[BlockLit3]] %[[ConstInt8]] %[[ConstInt8]]
  ;                            %[[LocalBuf31]]

;   enqueue_kernel(default_queue, flags, ndrange, 2, event_wait_list2, &clk_event,
;                  ^(local void *p) {
;                    return;
;                  },
;                  c);

  ; Emits global block literal and block kernel.

  ; CHECK-SPIRV: %[[LocalBuf41:[0-9]+]] = OpPtrAccessChain %[[Int32LocPtrTy]]
  ; CHECK-SPIRV: %[[LocalBuf42:[0-9]+]] = OpPtrAccessChain %[[Int32LocPtrTy]]
  ; CHECK-SPIRV: %[[LocalBuf43:[0-9]+]] = OpPtrAccessChain %[[Int32LocPtrTy]]
  ; CHECK-SPIRV: %[[BlockLit4Tmp:[0-9]+]] = OpBitcast %{{[0-9]+}} %[[BlockGlb2:[0-9]+]]
  ; CHECK-SPIRV: %[[BlockLit4:[0-9]+]] = OpPtrCastToGeneric %[[Int8PtrGenTy]] %[[BlockLit4Tmp]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpEnqueueKernel %[[Int32Ty]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ;                            %[[ConstInt0]] %[[EventNull]] %[[EventNull]]
  ;                            %[[BlockKer4]] %[[BlockLit4]] %[[ConstInt8]] %[[ConstInt8]]
  ;                            %[[LocalBuf41]] %[[LocalBuf42]] %[[LocalBuf43]]

;   enqueue_kernel(default_queue, flags, ndrange,
;                  ^(local void *p1, local void *p2, local void *p3) {
;                    return;
;                  },
;                  1, 2, 4);

  ; Emits block literal on stack and block kernel.

  ; CHECK-SPIRV: %[[Event1:[0-9]+]] = OpPtrCastToGeneric %[[EventPtrTy]]

  ; CHECK-SPIRV: %[[BlockLit2:[0-9]+]] = OpPtrCastToGeneric %[[Int8PtrGenTy]]
  ; CHECK-SPIRV: %{{[0-9]+}} = OpEnqueueKernel %[[Int32Ty]] %{{[0-9]+}} %{{[0-9]+}} %{{[0-9]+}}
  ;                            %[[ConstInt0]] %[[EventNull]] %[[Event1]]
  ;                            %[[BlockKer5]] %[[BlockLit5]] %[[ConstInt20]] %[[ConstInt8]]

;   enqueue_kernel(default_queue, flags, ndrange, 0, NULL, &clk_event,
;                  ^(void) {
;                    a[i] = b[i];
;                  });
; }

; CHECK-SPIRV-DAG: %[[BlockKer1]] = OpFunction %[[VoidTy]] None %[[BlockTy1]]
; CHECK-SPIRV-DAG: %[[BlockKer2]] = OpFunction %[[VoidTy]] None %[[BlockTy1]]
; CHECK-SPIRV-DAG: %[[BlockKer3]] = OpFunction %[[VoidTy]] None %[[BlockTy2]]
; CHECK-SPIRV-DAG: %[[BlockKer4]] = OpFunction %[[VoidTy]] None %[[BlockTy3]]
; CHECK-SPIRV-DAG: %[[BlockKer5]] = OpFunction %[[VoidTy]] None %[[BlockTy1]]

%opencl.queue_t = type opaque
%struct.ndrange_t = type { i32 }
%opencl.clk_event_t = type opaque
%struct.__opencl_block_literal_generic = type { i32, i32, i8 addrspace(4)* }

@__block_literal_global = internal addrspace(1) constant { i32, i32, i8 addrspace(4)* } { i32 12, i32 4, i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*, i8 addrspace(3)*)* @__device_side_enqueue_block_invoke_3 to i8*) to i8 addrspace(4)*) }, align 4 #0
@__block_literal_global.1 = internal addrspace(1) constant { i32, i32, i8 addrspace(4)* } { i32 12, i32 4, i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*, i8 addrspace(3)*, i8 addrspace(3)*, i8 addrspace(3)*)* @__device_side_enqueue_block_invoke_4 to i8*) to i8 addrspace(4)*) }, align 4 #0

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local spir_kernel void @device_side_enqueue(i32 addrspace(1)* noundef %a, i32 addrspace(1)* noundef %b, i32 noundef %i, i8 noundef signext %c0) #1 !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_type_qual !6 {
entry:
  %a.addr = alloca i32 addrspace(1)*, align 4
  %b.addr = alloca i32 addrspace(1)*, align 4
  %i.addr = alloca i32, align 4
  %c0.addr = alloca i8, align 1
  %default_queue = alloca %opencl.queue_t*, align 4
  %flags = alloca i32, align 4
  %ndrange = alloca %struct.ndrange_t, align 4
  %clk_event = alloca %opencl.clk_event_t*, align 4
  %event_wait_list = alloca %opencl.clk_event_t*, align 4
  %event_wait_list2 = alloca [1 x %opencl.clk_event_t*], align 4
  %tmp = alloca %struct.ndrange_t, align 4
  %block = alloca <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>, align 4
  %tmp3 = alloca %struct.ndrange_t, align 4
  %block4 = alloca <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, align 4
  %c = alloca i8, align 1
  %tmp11 = alloca %struct.ndrange_t, align 4
  %block_sizes = alloca [1 x i32], align 4
  %tmp12 = alloca %struct.ndrange_t, align 4
  %block_sizes13 = alloca [3 x i32], align 4
  %tmp14 = alloca %struct.ndrange_t, align 4
  %block15 = alloca <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, align 4
  store i32 addrspace(1)* %a, i32 addrspace(1)** %a.addr, align 4
  store i32 addrspace(1)* %b, i32 addrspace(1)** %b.addr, align 4
  store i32 %i, i32* %i.addr, align 4
  store i8 %c0, i8* %c0.addr, align 1
  store i32 0, i32* %flags, align 4
  %arrayinit.begin = getelementptr inbounds [1 x %opencl.clk_event_t*], [1 x %opencl.clk_event_t*]* %event_wait_list2, i32 0, i32 0
  %0 = load %opencl.clk_event_t*, %opencl.clk_event_t** %clk_event, align 4
  store %opencl.clk_event_t* %0, %opencl.clk_event_t** %arrayinit.begin, align 4
  %1 = load %opencl.queue_t*, %opencl.queue_t** %default_queue, align 4
  %2 = load i32, i32* %flags, align 4
  %3 = bitcast %struct.ndrange_t* %tmp to i8*
  %4 = bitcast %struct.ndrange_t* %ndrange to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* align 4 %3, i8* align 4 %4, i32 4, i1 false)
  %block.size = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>* %block, i32 0, i32 0
  store i32 21, i32* %block.size, align 4
  %block.align = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>* %block, i32 0, i32 1
  store i32 4, i32* %block.align, align 4
  %block.invoke = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>* %block, i32 0, i32 2
  store i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*)* @__device_side_enqueue_block_invoke to i8*) to i8 addrspace(4)*), i8 addrspace(4)** %block.invoke, align 4
  %block.captured = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>* %block, i32 0, i32 3
  %5 = load i32 addrspace(1)*, i32 addrspace(1)** %a.addr, align 4
  store i32 addrspace(1)* %5, i32 addrspace(1)** %block.captured, align 4
  %block.captured1 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>* %block, i32 0, i32 4
  %6 = load i32, i32* %i.addr, align 4
  store i32 %6, i32* %block.captured1, align 4
  %block.captured2 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>* %block, i32 0, i32 5
  %7 = load i8, i8* %c0.addr, align 1
  store i8 %7, i8* %block.captured2, align 4
  %8 = bitcast <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>* %block to %struct.__opencl_block_literal_generic*
  %9 = addrspacecast %struct.__opencl_block_literal_generic* %8 to i8 addrspace(4)*
  %10 = call spir_func i32 @__enqueue_kernel_basic(%opencl.queue_t* %1, i32 %2, %struct.ndrange_t* byval(%struct.ndrange_t) %tmp, i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*)* @__device_side_enqueue_block_invoke_kernel to i8*) to i8 addrspace(4)*), i8 addrspace(4)* %9)
  %11 = load %opencl.queue_t*, %opencl.queue_t** %default_queue, align 4
  %12 = load i32, i32* %flags, align 4
  %13 = bitcast %struct.ndrange_t* %tmp3 to i8*
  %14 = bitcast %struct.ndrange_t* %ndrange to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* align 4 %13, i8* align 4 %14, i32 4, i1 false)
  %15 = addrspacecast %opencl.clk_event_t** %event_wait_list to %opencl.clk_event_t* addrspace(4)*
  %16 = addrspacecast %opencl.clk_event_t** %clk_event to %opencl.clk_event_t* addrspace(4)*
  %block.size5 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block4, i32 0, i32 0
  store i32 24, i32* %block.size5, align 4
  %block.align6 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block4, i32 0, i32 1
  store i32 4, i32* %block.align6, align 4
  %block.invoke7 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block4, i32 0, i32 2
  store i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*)* @__device_side_enqueue_block_invoke_2 to i8*) to i8 addrspace(4)*), i8 addrspace(4)** %block.invoke7, align 4
  %block.captured8 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block4, i32 0, i32 3
  %17 = load i32 addrspace(1)*, i32 addrspace(1)** %a.addr, align 4
  store i32 addrspace(1)* %17, i32 addrspace(1)** %block.captured8, align 4
  %block.captured9 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block4, i32 0, i32 4
  %18 = load i32, i32* %i.addr, align 4
  store i32 %18, i32* %block.captured9, align 4
  %block.captured10 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block4, i32 0, i32 5
  %19 = load i32 addrspace(1)*, i32 addrspace(1)** %b.addr, align 4
  store i32 addrspace(1)* %19, i32 addrspace(1)** %block.captured10, align 4
  %20 = bitcast <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block4 to %struct.__opencl_block_literal_generic*
  %21 = addrspacecast %struct.__opencl_block_literal_generic* %20 to i8 addrspace(4)*
  %22 = call spir_func i32 @__enqueue_kernel_basic_events(%opencl.queue_t* %11, i32 %12, %struct.ndrange_t* %tmp3, i32 2, %opencl.clk_event_t* addrspace(4)* %15, %opencl.clk_event_t* addrspace(4)* %16, i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*)* @__device_side_enqueue_block_invoke_2_kernel to i8*) to i8 addrspace(4)*), i8 addrspace(4)* %21)
  %23 = load %opencl.queue_t*, %opencl.queue_t** %default_queue, align 4
  %24 = load i32, i32* %flags, align 4
  %25 = bitcast %struct.ndrange_t* %tmp11 to i8*
  %26 = bitcast %struct.ndrange_t* %ndrange to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* align 4 %25, i8* align 4 %26, i32 4, i1 false)
  %arraydecay = getelementptr inbounds [1 x %opencl.clk_event_t*], [1 x %opencl.clk_event_t*]* %event_wait_list2, i32 0, i32 0
  %27 = addrspacecast %opencl.clk_event_t** %arraydecay to %opencl.clk_event_t* addrspace(4)*
  %28 = addrspacecast %opencl.clk_event_t** %clk_event to %opencl.clk_event_t* addrspace(4)*
  %29 = getelementptr [1 x i32], [1 x i32]* %block_sizes, i32 0, i32 0
  %30 = load i8, i8* %c, align 1
  %31 = zext i8 %30 to i32
  store i32 %31, i32* %29, align 4
  %32 = call spir_func i32 @__enqueue_kernel_events_varargs(%opencl.queue_t* %23, i32 %24, %struct.ndrange_t* %tmp11, i32 2, %opencl.clk_event_t* addrspace(4)* %27, %opencl.clk_event_t* addrspace(4)* %28, i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*, i8 addrspace(3)*)* @__device_side_enqueue_block_invoke_3_kernel to i8*) to i8 addrspace(4)*), i8 addrspace(4)* addrspacecast (i8 addrspace(1)* bitcast ({ i32, i32, i8 addrspace(4)* } addrspace(1)* @__block_literal_global to i8 addrspace(1)*) to i8 addrspace(4)*), i32 1, i32* %29)
  %33 = load %opencl.queue_t*, %opencl.queue_t** %default_queue, align 4
  %34 = load i32, i32* %flags, align 4
  %35 = bitcast %struct.ndrange_t* %tmp12 to i8*
  %36 = bitcast %struct.ndrange_t* %ndrange to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* align 4 %35, i8* align 4 %36, i32 4, i1 false)
  %37 = getelementptr [3 x i32], [3 x i32]* %block_sizes13, i32 0, i32 0
  store i32 1, i32* %37, align 4
  %38 = getelementptr [3 x i32], [3 x i32]* %block_sizes13, i32 0, i32 1
  store i32 2, i32* %38, align 4
  %39 = getelementptr [3 x i32], [3 x i32]* %block_sizes13, i32 0, i32 2
  store i32 4, i32* %39, align 4
  %40 = call spir_func i32 @__enqueue_kernel_varargs(%opencl.queue_t* %33, i32 %34, %struct.ndrange_t* %tmp12, i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*, i8 addrspace(3)*, i8 addrspace(3)*, i8 addrspace(3)*)* @__device_side_enqueue_block_invoke_4_kernel to i8*) to i8 addrspace(4)*), i8 addrspace(4)* addrspacecast (i8 addrspace(1)* bitcast ({ i32, i32, i8 addrspace(4)* } addrspace(1)* @__block_literal_global.1 to i8 addrspace(1)*) to i8 addrspace(4)*), i32 3, i32* %37)
  %41 = load %opencl.queue_t*, %opencl.queue_t** %default_queue, align 4
  %42 = load i32, i32* %flags, align 4
  %43 = bitcast %struct.ndrange_t* %tmp14 to i8*
  %44 = bitcast %struct.ndrange_t* %ndrange to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* align 4 %43, i8* align 4 %44, i32 4, i1 false)
  %45 = addrspacecast %opencl.clk_event_t** %clk_event to %opencl.clk_event_t* addrspace(4)*
  %block.size16 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block15, i32 0, i32 0
  store i32 24, i32* %block.size16, align 4
  %block.align17 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block15, i32 0, i32 1
  store i32 4, i32* %block.align17, align 4
  %block.invoke18 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block15, i32 0, i32 2
  store i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*)* @__device_side_enqueue_block_invoke_5 to i8*) to i8 addrspace(4)*), i8 addrspace(4)** %block.invoke18, align 4
  %block.captured19 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block15, i32 0, i32 3
  %46 = load i32 addrspace(1)*, i32 addrspace(1)** %a.addr, align 4
  store i32 addrspace(1)* %46, i32 addrspace(1)** %block.captured19, align 4
  %block.captured20 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block15, i32 0, i32 4
  %47 = load i32, i32* %i.addr, align 4
  store i32 %47, i32* %block.captured20, align 4
  %block.captured21 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block15, i32 0, i32 5
  %48 = load i32 addrspace(1)*, i32 addrspace(1)** %b.addr, align 4
  store i32 addrspace(1)* %48, i32 addrspace(1)** %block.captured21, align 4
  %49 = bitcast <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>* %block15 to %struct.__opencl_block_literal_generic*
  %50 = addrspacecast %struct.__opencl_block_literal_generic* %49 to i8 addrspace(4)*
  %51 = call spir_func i32 @__enqueue_kernel_basic_events(%opencl.queue_t* %41, i32 %42, %struct.ndrange_t* %tmp14, i32 0, %opencl.clk_event_t* addrspace(4)* null, %opencl.clk_event_t* addrspace(4)* %45, i8 addrspace(4)* addrspacecast (i8* bitcast (void (i8 addrspace(4)*)* @__device_side_enqueue_block_invoke_5_kernel to i8*) to i8 addrspace(4)*), i8 addrspace(4)* %50)
  ret void
}

; Function Attrs: argmemonly nofree nounwind willreturn
declare void @llvm.memcpy.p0i8.p0i8.i32(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i32, i1 immarg) #2

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func void @__device_side_enqueue_block_invoke(i8 addrspace(4)* noundef %.block_descriptor) #3 {
entry:
  %.block_descriptor.addr = alloca i8 addrspace(4)*, align 4
  %block.addr = alloca <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }> addrspace(4)*, align 4
  store i8 addrspace(4)* %.block_descriptor, i8 addrspace(4)** %.block_descriptor.addr, align 4
  %block = bitcast i8 addrspace(4)* %.block_descriptor to <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }> addrspace(4)*
  store <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }> addrspace(4)* %block, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }> addrspace(4)** %block.addr, align 4
  %block.capture.addr = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }> addrspace(4)* %block, i32 0, i32 5
  %0 = load i8, i8 addrspace(4)* %block.capture.addr, align 4
  %conv = sext i8 %0 to i32
  %block.capture.addr1 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }> addrspace(4)* %block, i32 0, i32 3
  %1 = load i32 addrspace(1)*, i32 addrspace(1)* addrspace(4)* %block.capture.addr1, align 4
  %block.capture.addr2 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i8 }> addrspace(4)* %block, i32 0, i32 4
  %2 = load i32, i32 addrspace(4)* %block.capture.addr2, align 4
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i32 %2
  store i32 %conv, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: nounwind
define spir_kernel void @__device_side_enqueue_block_invoke_kernel(i8 addrspace(4)* %0) #4 {
entry:
  call spir_func void @__device_side_enqueue_block_invoke(i8 addrspace(4)* %0)
  ret void
}

declare spir_func i32 @__enqueue_kernel_basic(%opencl.queue_t*, i32, %struct.ndrange_t*, i8 addrspace(4)*, i8 addrspace(4)*)

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func void @__device_side_enqueue_block_invoke_2(i8 addrspace(4)* noundef %.block_descriptor) #3 {
entry:
  %.block_descriptor.addr = alloca i8 addrspace(4)*, align 4
  %block.addr = alloca <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)*, align 4
  store i8 addrspace(4)* %.block_descriptor, i8 addrspace(4)** %.block_descriptor.addr, align 4
  %block = bitcast i8 addrspace(4)* %.block_descriptor to <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)*
  store <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)* %block, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)** %block.addr, align 4
  %block.capture.addr = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)* %block, i32 0, i32 5
  %0 = load i32 addrspace(1)*, i32 addrspace(1)* addrspace(4)* %block.capture.addr, align 4
  %block.capture.addr1 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)* %block, i32 0, i32 4
  %1 = load i32, i32 addrspace(4)* %block.capture.addr1, align 4
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i32 %1
  %2 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %block.capture.addr2 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)* %block, i32 0, i32 3
  %3 = load i32 addrspace(1)*, i32 addrspace(1)* addrspace(4)* %block.capture.addr2, align 4
  %block.capture.addr3 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)* %block, i32 0, i32 4
  %4 = load i32, i32 addrspace(4)* %block.capture.addr3, align 4
  %arrayidx4 = getelementptr inbounds i32, i32 addrspace(1)* %3, i32 %4
  store i32 %2, i32 addrspace(1)* %arrayidx4, align 4
  ret void
}

; Function Attrs: nounwind
define spir_kernel void @__device_side_enqueue_block_invoke_2_kernel(i8 addrspace(4)* %0) #4 {
entry:
  call spir_func void @__device_side_enqueue_block_invoke_2(i8 addrspace(4)* %0)
  ret void
}

declare spir_func i32 @__enqueue_kernel_basic_events(%opencl.queue_t*, i32, %struct.ndrange_t*, i32, %opencl.clk_event_t* addrspace(4)*, %opencl.clk_event_t* addrspace(4)*, i8 addrspace(4)*, i8 addrspace(4)*)

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func void @__device_side_enqueue_block_invoke_3(i8 addrspace(4)* noundef %.block_descriptor, i8 addrspace(3)* noundef %p) #3 {
entry:
  %.block_descriptor.addr = alloca i8 addrspace(4)*, align 4
  %p.addr = alloca i8 addrspace(3)*, align 4
  %block.addr = alloca <{ i32, i32, i8 addrspace(4)* }> addrspace(4)*, align 4
  store i8 addrspace(4)* %.block_descriptor, i8 addrspace(4)** %.block_descriptor.addr, align 4
  %block = bitcast i8 addrspace(4)* %.block_descriptor to <{ i32, i32, i8 addrspace(4)* }> addrspace(4)*
  store i8 addrspace(3)* %p, i8 addrspace(3)** %p.addr, align 4
  store <{ i32, i32, i8 addrspace(4)* }> addrspace(4)* %block, <{ i32, i32, i8 addrspace(4)* }> addrspace(4)** %block.addr, align 4
  ret void
}

; Function Attrs: nounwind
define spir_kernel void @__device_side_enqueue_block_invoke_3_kernel(i8 addrspace(4)* %0, i8 addrspace(3)* %1) #4 {
entry:
  call spir_func void @__device_side_enqueue_block_invoke_3(i8 addrspace(4)* %0, i8 addrspace(3)* %1)
  ret void
}

declare spir_func i32 @__enqueue_kernel_events_varargs(%opencl.queue_t*, i32, %struct.ndrange_t*, i32, %opencl.clk_event_t* addrspace(4)*, %opencl.clk_event_t* addrspace(4)*, i8 addrspace(4)*, i8 addrspace(4)*, i32, i32*)

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func void @__device_side_enqueue_block_invoke_4(i8 addrspace(4)* noundef %.block_descriptor, i8 addrspace(3)* noundef %p1, i8 addrspace(3)* noundef %p2, i8 addrspace(3)* noundef %p3) #3 {
entry:
  %.block_descriptor.addr = alloca i8 addrspace(4)*, align 4
  %p1.addr = alloca i8 addrspace(3)*, align 4
  %p2.addr = alloca i8 addrspace(3)*, align 4
  %p3.addr = alloca i8 addrspace(3)*, align 4
  %block.addr = alloca <{ i32, i32, i8 addrspace(4)* }> addrspace(4)*, align 4
  store i8 addrspace(4)* %.block_descriptor, i8 addrspace(4)** %.block_descriptor.addr, align 4
  %block = bitcast i8 addrspace(4)* %.block_descriptor to <{ i32, i32, i8 addrspace(4)* }> addrspace(4)*
  store i8 addrspace(3)* %p1, i8 addrspace(3)** %p1.addr, align 4
  store i8 addrspace(3)* %p2, i8 addrspace(3)** %p2.addr, align 4
  store i8 addrspace(3)* %p3, i8 addrspace(3)** %p3.addr, align 4
  store <{ i32, i32, i8 addrspace(4)* }> addrspace(4)* %block, <{ i32, i32, i8 addrspace(4)* }> addrspace(4)** %block.addr, align 4
  ret void
}

; Function Attrs: nounwind
define spir_kernel void @__device_side_enqueue_block_invoke_4_kernel(i8 addrspace(4)* %0, i8 addrspace(3)* %1, i8 addrspace(3)* %2, i8 addrspace(3)* %3) #4 {
entry:
  call spir_func void @__device_side_enqueue_block_invoke_4(i8 addrspace(4)* %0, i8 addrspace(3)* %1, i8 addrspace(3)* %2, i8 addrspace(3)* %3)
  ret void
}

declare spir_func i32 @__enqueue_kernel_varargs(%opencl.queue_t*, i32, %struct.ndrange_t*, i8 addrspace(4)*, i8 addrspace(4)*, i32, i32*)

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func void @__device_side_enqueue_block_invoke_5(i8 addrspace(4)* noundef %.block_descriptor) #3 {
entry:
  %.block_descriptor.addr = alloca i8 addrspace(4)*, align 4
  %block.addr = alloca <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)*, align 4
  store i8 addrspace(4)* %.block_descriptor, i8 addrspace(4)** %.block_descriptor.addr, align 4
  %block = bitcast i8 addrspace(4)* %.block_descriptor to <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)*
  store <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)* %block, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)** %block.addr, align 4
  %block.capture.addr = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)* %block, i32 0, i32 5
  %0 = load i32 addrspace(1)*, i32 addrspace(1)* addrspace(4)* %block.capture.addr, align 4
  %block.capture.addr1 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)* %block, i32 0, i32 4
  %1 = load i32, i32 addrspace(4)* %block.capture.addr1, align 4
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i32 %1
  %2 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %block.capture.addr2 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)* %block, i32 0, i32 3
  %3 = load i32 addrspace(1)*, i32 addrspace(1)* addrspace(4)* %block.capture.addr2, align 4
  %block.capture.addr3 = getelementptr inbounds <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }>, <{ i32, i32, i8 addrspace(4)*, i32 addrspace(1)*, i32, i32 addrspace(1)* }> addrspace(4)* %block, i32 0, i32 4
  %4 = load i32, i32 addrspace(4)* %block.capture.addr3, align 4
  %arrayidx4 = getelementptr inbounds i32, i32 addrspace(1)* %3, i32 %4
  store i32 %2, i32 addrspace(1)* %arrayidx4, align 4
  ret void
}

; Function Attrs: nounwind
define spir_kernel void @__device_side_enqueue_block_invoke_5_kernel(i8 addrspace(4)* %0) #4 {
entry:
  call spir_func void @__device_side_enqueue_block_invoke_5(i8 addrspace(4)* %0)
  ret void
}

attributes #0 = { "objc_arc_inert" }
attributes #1 = { convergent noinline norecurse nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="false" }
attributes #2 = { argmemonly nofree nounwind willreturn }
attributes #3 = { convergent noinline nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
!3 = !{i32 1, i32 1, i32 0, i32 0}
!4 = !{!"none", !"none", !"none", !"none"}
!5 = !{!"int*", !"int*", !"int", !"char"}
!6 = !{!"", !"", !"", !""}
