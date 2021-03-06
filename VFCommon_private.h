/*
 VFCommon_private.h
 This has to be the stupidest name in the world...
 We'll have to deal with it until the project is completely reorganized

 (c) 2010-2013 Blaise Bourdin bourdin@lsu.edu
*/
#if !defined( VF_Standalone_VFCommon_private_h)
#define VF_Standalone_VFCommon_private_h

static const char *VFMechSolverName[] = {
	"FRACTURE",
	"ELASTICITY",
	"NOMECH",
	"VFMechSolverName",
	"",
	0
};

static const char *VFUnilateralName[] = {
	"NONE",
	"NOCOMPRESSION",
	"VFUnilateralName",
	"",
	0
};

static const char *VFFlowSolverName[] = {
	"FLOWSOLVER_SNESSTANDARDFEM",
	"FLOWSOLVER_TS",
	"FLOWSOLVER_SNES",
	"FLOWSOLVER_FEM",
	"FLOWSOLVER_KSPMIXEDFEM",
	"FLOWSOLVER_SNESMIXEDFEM",
	"FLOWSOLVER_TSMIXEDFEM",
	"FAKE",
	"READFROMFILES",
	"FLOWSOLVER_NONE",
	"VFFlowSolverName",
	"",
	0
};

static const char *ResFlowMechCouplingName[] = {
	"FIXEDSTRAIN",
	"FIXEDSTRESS",
  "ResFlowMechCouplingName",
	"",
	0
};

static const char *VFHeatSolverName[] = {
	"HEATSOLVER_SNESFEM",
	"VFHeatSolverName",
	"",
	0
};

static const char *VFFileFormatName[] = {
	"bin",
  "vtk",
	"VFFileFormatName",
	"",
	0
};
#endif
