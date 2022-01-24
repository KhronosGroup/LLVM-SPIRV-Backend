; RUN: llc -O0 %s -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; ModuleID = 'GenericCastToPtr.cl'
source_filename = "GenericCastToPtr.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spirv32-unknown-unknown"

; CHECK-SPIRV: OpGenericCastToPtr

; global short2 *testGenericCastToPtrGlobal(generic short2 *a) {
;   return (global short2 *)a;
; }

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn
define dso_local spir_func <2 x i16> addrspace(1)* @testGenericCastToPtrGlobal(<2 x i16> addrspace(4)* noundef readnone %a) local_unnamed_addr #0 {
entry:
  %0 = addrspacecast <2 x i16> addrspace(4)* %a to <2 x i16> addrspace(1)*
  ret <2 x i16> addrspace(1)* %0
}

; CHECK-SPIRV: OpGenericCastToPtr

; local short2 *testGenericCastToPtrLocal(generic short2 *a) {
;   return (local short2 *)a;
; }

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn
define dso_local spir_func <2 x i16> addrspace(3)* @testGenericCastToPtrLocal(<2 x i16> addrspace(4)* noundef readnone %a) local_unnamed_addr #0 {
entry:
  %0 = addrspacecast <2 x i16> addrspace(4)* %a to <2 x i16> addrspace(3)*
  ret <2 x i16> addrspace(3)* %0
}

; CHECK-SPIRV: OpGenericCastToPtr

; private short2 *testGenericCastToPtrPrivate(generic short2 *a) {
;   return (private short2 *)a;
; }

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn
define dso_local spir_func <2 x i16>* @testGenericCastToPtrPrivate(<2 x i16> addrspace(4)* noundef readnone %a) local_unnamed_addr #0 {
entry:
  %0 = addrspacecast <2 x i16> addrspace(4)* %a to <2 x i16>*
  ret <2 x i16>* %0
}

; CHECK-SPIRV: OpGenericCastToPtrExplicit

; global short2 *testGenericCastToPtrExplicitGlobal(generic short2 *a) {
;   return to_global(a);
; }

; Function Attrs: norecurse nounwind
define dso_local spir_func <2 x i16> addrspace(1)* @testGenericCastToPtrExplicitGlobal(<2 x i16> addrspace(4)* noundef %a) local_unnamed_addr #1 {
entry:
  %0 = bitcast <2 x i16> addrspace(4)* %a to i8 addrspace(4)*
  %1 = call spir_func i8 addrspace(1)* @__to_global(i8 addrspace(4)* %0) #2
  %2 = bitcast i8 addrspace(1)* %1 to <2 x i16> addrspace(1)*
  ret <2 x i16> addrspace(1)* %2
}

declare spir_func i8 addrspace(1)* @__to_global(i8 addrspace(4)*) local_unnamed_addr

; CHECK-SPIRV: OpGenericCastToPtrExplicit

; local short2 *testGenericCastToPtrExplicitLocal(generic short2 *a) {
;   return to_local(a);
; }

; Function Attrs: norecurse nounwind
define dso_local spir_func <2 x i16> addrspace(3)* @testGenericCastToPtrExplicitLocal(<2 x i16> addrspace(4)* noundef %a) local_unnamed_addr #1 {
entry:
  %0 = bitcast <2 x i16> addrspace(4)* %a to i8 addrspace(4)*
  %1 = call spir_func i8 addrspace(3)* @__to_local(i8 addrspace(4)* %0) #2
  %2 = bitcast i8 addrspace(3)* %1 to <2 x i16> addrspace(3)*
  ret <2 x i16> addrspace(3)* %2
}

declare spir_func i8 addrspace(3)* @__to_local(i8 addrspace(4)*) local_unnamed_addr

; CHECK-SPIRV: OpGenericCastToPtrExplicit

; private short2 *testGenericCastToPtrExplicitPrivate(generic short2 *a) {
;   return to_private(a);
; }

; Function Attrs: norecurse nounwind
define dso_local spir_func <2 x i16>* @testGenericCastToPtrExplicitPrivate(<2 x i16> addrspace(4)* noundef %a) local_unnamed_addr #1 {
entry:
  %0 = bitcast <2 x i16> addrspace(4)* %a to i8 addrspace(4)*
  %1 = call spir_func i8* @__to_private(i8 addrspace(4)* %0) #2
  %2 = bitcast i8* %1 to <2 x i16>*
  ret <2 x i16>* %2
}

declare spir_func i8* @__to_private(i8 addrspace(4)*) local_unnamed_addr

attributes #0 = { mustprogress nofree norecurse nosync nounwind readnone willreturn "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 2, i32 0}
!2 = !{!"clang version 14.0.0 (https://github.com/llvm/llvm-project.git 881b6a009fb6e2dd5fb924524cd6eacd14148a08)"}
