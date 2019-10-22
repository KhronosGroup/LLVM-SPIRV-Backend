SPIR-V Backend
==============

Description
-----------

An experimental SPIR-V backend for LLVM based on GlobalISel.

While this is a work-in-progress, this backend already supports a significant
subset of LLVM IR as input. Refer to the test suite to get an idea about the
coverage.

SPIR-V specification:
https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html

Build and Testing Instructions
------------------------------

Cmake configuration:

.. code-block:: bash

        cd /path/to/build
        cmake -G Ninja -DLLVM_TARGETS_TO_BUILD="SPIRV" /path/to/checkout/llvm

Optional CMake options (among others):

-  -DLLVM\_CCACHE\_BUILD=ON
-  -DCMAKE\_BUILD\_TYPE=Debug
-  -DBUILD\_SHARED\_LIBS=ON
-  -DLLVM_ENABLE_SPHINX=ON -DSPHINX_WARNINGS_AS_ERRORS=OFF
-  -DLLVM_ENABLE_PROJECTS="clang"

This will also create llvm-lit in /path/to/build/bin.

Command to build llc and all the tools needed for unit testing:

.. code-block:: bash

        cmake --build /path/to/build -- llc FileCheck count not llvm-config

Running tests for the SPIR-V backend:

.. code-block:: bash

        /path/to/build/bin/llvm-lit llvm/test/CodeGen/SPIRV

TODO This currently emits many warnings about missing (but unused)
tools. Can we silence that (without building those)?

When Sphinx is enabled in CMake, you can build this documentation using:

.. code-block:: bash

        cmake --build /path/to/build -- docs-llvm-html

In the example below, we use Clang. If you wish not to build it yourself, Clang
9 can be used instead. To build it yourself, use the usual command:

.. code-block:: bash

        cmake --build /path/to/build -- clang

Usage Example
-------------

Here is an example on how to compile an OpenCL program to SPIR-V. We
first use Clang to compile OpenCL into LLVM IR with -emit-llvm -S. We
also patch the tripple in the generated LLVM IR with Sed to make it
compatible with the SPIR-V backend.

.. code-block:: bash

        echo '
        constant size_t N = 16;

        kernel void example(global float* in, global float* out) {
          size_t index = get_global_id(0);
          for (size_t i = 0; i < N; ++i) {
            out[index + i] = exp(in[index + i]);
          }
        }
        ' > example.cl

        /path/to/build/bin/clang -c example.cl -x cl -cl-std=cl2.0 -O0 \
                                 -Xclang -finclude-default-header \
                                 -emit-llvm -S -target spir -o - \
        | sed -Ee 's#target triple = "spir"#target triple = "spirv32-unknown-unknown"#' \
        > example.ll

        /path/to/build/bin/llc example.ll -o example.spvt

The generated SPIR-V is in textual format. Support for binary format
needs to be added. Furthermore, a few things need to be improved, such as
adding the SPIR-V Magic Number, version number, etc...

Backend Structure
-----------------

Overview
~~~~~~~~

The source code lives mainly in llvm/lib/Target/SPIRV and the tests are in
llvm/test/CodeGen/SPIRV.

The details available here are mainly to make it easier to dive into the
code. These details also mention a few places where the code can be
improved or completed.

Changes to GlobalISel
~~~~~~~~~~~~~~~~~~~~~

Currently, the backend relies on some (hopefully minor) modifications of
the IRTranslator (llvm/include/llvm/CodeGen/GlobalISel/IRTranslator.h).

Making the methods "protected", and marking the ones that need altered
for SPIR-V as "virtual" seemed like the way to document what needs to be
changed in the interface with minimal diffs to the original, and no
changes to the base IRTranslator implementation.

The main changes required are:

-  Having some kind of call-back whenever a vreg is created for an LLVM
   IR Value so type information can be preserved for the value.
-  Having the option to avoid flattening structs and aggregate types
-  Callbacks for creating constants (to e.g. have correctly typed
   nullptrs, global OpVariables with initializer, constant structs via
   OpCompositeConstruct etc.
-  Having a way to preserve indices (e.g. in extract element, get
   element pointer), rather than using pointer offsets into a flattened
   struct (which might potentially have padding), which becomes
   difficult to map to OpAccessChain, OpCompositeExtract etc. which use
   offsets.
-  Add an option to avoid turning bitcasts into copies (to preserve type
   information)
-  Add a way determine between explicitly defined alignments, and ones
   implicitly calculated from the data types.

Backend Configuration Points
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Relevant bits responsible for the backend configuration and behavior:

-  SPIRVTargetMachine

   This class sets up the backend to use GlobalISel, defines the passes
   using SPIRVPassConfig, the data-layout, etc...

-  SPIRVSubtarget

   The TargetSubtargetInfo, referred by SPIRVTargetMachine, that defines
   various bits of information about the SPIR-V environment (e.g. OpenCL
   vs Vulkan, pointer size, ...), which instruction selector or
   legalizer are used, etc...

   Currently, SPIRVSubtarget contains a few placeholders, such as the
   set of available extensions, that should be defined based on command
   line arguments, or some other mechanism.

-  SPIRVInstrInfo

   This class, aided by TableGen, defines and describes SPIR-V OpCodes.
   This is currently hand-crafted with most SPIR-V OpCodes but could be
   further scripted. See SPIRVInstrInfo.(h\|cpp\|td) for more details.

-  SPIRVRegisterInfo and SPIRVRegisterBankInfo

   These two classes are used to define information about registers for
   SPIR-V. For SPIR-V, we only need very few register banks and classes.
   Actually, we only need two: one for types and one for identifiers.

-  SPIRVTypeRegistry

   SPIRVTypeRegistry is used to maintain rich type information required
   for SPIR-V even after lowering from LLVM IR to GMIR. It can convert
   an llvm::Type into an OpTypeXXX instruction, and map it to a virtual
   register using and ASSIGN\_TYPE pseudo instruction.

   Type info from this class can only be used before it gets stripped
   out by the InstructionSelector stage. All type info is function-local
   until the final SPIRVGlobalTypesAndRegNums pass hoists it globally
   and deduplicates it all.

-  SPIRVLegalizerInfo

   This class defines which operations are valid for SPIR-V.

Execution Pipeline
~~~~~~~~~~~~~~~~~~

The main functions being executed, in that order:

-  SPIRVBasicBlockDominance::runOnFunction

   Sort the Basic Blocks so that blocks appear before all blocks they
   dominate, as to satisfy SPIR-V validation rules.

-  SPIRVIRTranslator::runOnMachineFunction

   SPIRVIRTranslator overrides GlobalISel's IRTranslator to customize
   the default lowering behavior. This is mostly to stop aggregate
   types like Structs from getting expanded into scalars, and to
   maintain type information required for SPIR-V using the
   SPIRVTypeRegistry before it gets discarded as LLVM IR is lowered to
   GMIR.

   SPIRVIRTranslator is relying on SPIRVCallLowering to lower LLVM calls
   into machine code calls. It relies on generateOpenCLBuiltinCall
   (SPIRVOpenCLBIFs.cpp) for anything specific to OpenCL. Similarly,
   SPIRVTypeRegistry will rely on generateOpenCLOpaqueType (in that same
   file) for any type specific to OpenCL.

-  Legalizer::runOnMachineFunction

   This SPIR-V backend relies on the default Legalizer and
   SPIRVLegalizerInfo.

-  SPIRVInstructionSelect::runOnMachineFunction

   SPIRVInstructionSelect is a custom subclass of InstructionSelect,
   which is mostly the same except from not requiring RegBankSelect to
   occur previously, and making sure a SPIRVTypeRegistry is initialized
   before and reset after each run.

   SPIRVInstructionSelect is helped by SPIRVInstructionSelector to
   generate SPIR-V instructions from target-independent instructions.

-  SPIRVBlockLabeler::runOnMachineFunction

   SPIRVBlockLabeler ensures all basic blocks start with an OpLabel, and
   ends with a suitable terminator.

-  SPIRVAddRequirements::runOnMachineFunction

   SPIRVAddRequirements iterates over all instructions and inserts the
   required OpCapability or OpExtension instructions.

-  SPIRVGlobalTypesAndRegNum::runOnModule

   SPIRVGlobalTypesAndRegNum hoists all the required function-local
   instructions to global scope, de-duplicating them where necessary.

   Before this pass, all SPIR-V instructions were local scope, and
   registers were numbered function-locally. However, SPIR-V requires
   Type instructions, global variables, capabilities, annotations etc.
   to be in global scope and occur at the start of the file. This pass
   hoists these as necessary.

   This pass also re-numbers the registers globally, and patches up any
   references to previously local registers that were hoisted, or
   function IDs which require globally scoped registers.

   This pass breaks all notion of register def/use, and generated
   MachineInstrs that are technically invalid as a result. As such, it
   must be the last pass, and requires instruction verification to be
   disabled afterwards.

-  AsmPrinter::runOnMachineFunction

   The AsmPrinter is customised for SPIR-V through the SPIRVAsmPrinter
   class. It is further customised through SPIRVMCInstLower and
   SPIRVInstPrinter.

Missing Features
~~~~~~~~~~~~~~~~

MCSPIRVStreamer currently subclasses MCObjectStreamer to disable all
processing. This should be modified in order to produce binary SPIR-V in
addition to the human readable SPIR-V output.

TODOs and FIXMEs can be found in various places in the code. Use this
command to get a list of the relevant ones:

.. code-block:: bash

        git grep -Ei '(FIXME|TODO)' -- llvm/lib/Target/SPIRV/ llvm/test/CodeGen/SPIRV/

Additionally, the following errors are known and should be fixed:

::

      LLVM ERROR: Cannot translate OpenCL built-in func: dot(float vector[4], float vector[4])
      LLVM ERROR: Cannot translate OpenCL built-in func: hadd(long vector[3], long vector[3])
      LLVM ERROR: Cannot translate OpenCL built-in func: logb(float vector[2])
      LLVM ERROR: Cannot translate OpenCL built-in func: isequal(float, float)
      LLVM ERROR: Cannot translate OpenCL built-in func: vload4(unsigned int, float const AS4*)
      LLVM ERROR: Cannot handle OpenCL atomic func: atomic_xchg

Integration tests are welcome contributions, too.

It would also be interesting to set up some fuzz testing, maybe based on
llvm-isel-fuzzer (see :doc:`Fuzzing LLVM <FuzzingLLVM>`).
