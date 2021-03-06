/*
 test12.c:
 Crack initiation and propagation under monotonically increasing tension, sliding and shear forces respectively.
 
 (c) 2010-2012 chukwudi Chukwudozie cchukw1@tigers.lsu.edu
 */

#include "petsc.h"
#include "CartFE.h"
#include "VFCommon.h"
#include "VFMech.h"
#include "VFFlow.h"

VFCtx    ctx;
VFFields fields;

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
	VFCtx          ctx;
	VFFields       fields;
	PetscErrorCode ierr;
	
	PetscReal length      = .2;
	PetscInt  orientation = 1;
	PetscInt  nopts       = 3;
	PetscInt  t,i,j,k,nx,ny,nz,xs,xm,ys,ym,zs,zm;
	PetscReal ****coords_array;
	PetscReal ****bcu_array;
	PetscReal BBmin[3],BBmax[3];
	PetscReal ElasticEnergy = 0;
	PetscReal InsituWork    = 0;
	PetscReal SurfaceEnergy = 0;
	char      filename[FILENAME_MAX];
	PetscReal bc = .005;
	PetscReal ***v_array;
	PetscReal lx,ly,lz;
	PetscReal ***pmult_array;
	PetscReal ****vfperm_array;
	PetscInt			altminit=1;
	Vec					Vold;
	PetscReal			errV=1e+10;
	
	ierr = PetscInitialize(&argc,&argv,(char*)0,banner);CHKERRQ(ierr);
	ierr = VFInitialize(&ctx,&fields);CHKERRQ(ierr);

	ierr = PetscOptionsGetInt(NULL,"-orientation",&orientation,NULL);CHKERRQ(ierr);
	ierr = PetscOptionsGetReal(NULL,"-bcU",&bc,NULL);CHKERRQ(ierr);
	ierr = DMDAGetInfo(ctx.daScal,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
					   NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
	ierr = DMDAGetCorners(ctx.daScal,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
	ierr = DMDAGetBoundingBox(ctx.daVect,BBmin,BBmax);CHKERRQ(ierr);
	ierr = VecDuplicate(fields.V,&Vold);CHKERRQ(ierr);
	lz = BBmax[2];
	ly = BBmax[1];
	lx = BBmax[0];	
	ierr = PetscPrintf(PETSC_COMM_WORLD,"Bounding box: %e, %e, %e\n",lx,ly,lz);CHKERRQ(ierr);
	ctx.matprop[0].beta  = 0.;
	ctx.matprop[0].alpha = 0.;
	ctx.timestep  = 1;
	ctx.maxtimestep = 2;
	
	ierr = DMDAVecGetArrayDOF(ctx.daVect,ctx.coordinates,&coords_array);CHKERRQ(ierr);
	ierr = VecSet(fields.V,1.0);CHKERRQ(ierr);
	ierr = VecSet(fields.VIrrev,1.0);CHKERRQ(ierr);
	ierr = VecSet(fields.U,0.0);CHKERRQ(ierr);
	ierr = VecSet(fields.theta,0.0);CHKERRQ(ierr);
	ierr = VecSet(fields.thetaRef,0.0);CHKERRQ(ierr);
	ierr = VecSet(fields.pressure,0.0);CHKERRQ(ierr);
	ierr = VecSet(fields.pressureRef,0.0);CHKERRQ(ierr);
	ierr = DMDAVecGetArray(ctx.daScal,fields.VIrrev,&v_array);CHKERRQ(ierr);
	for (i = 0; i < 6; i++) {
		ctx.bcV[0].face[i] = NONE;
		for (j = 0; j < 3; j++) {
			ctx.bcU[j].face[i] = NONE;
		}
	}
	for (i = 0; i < 12; i++) {
		ctx.bcV[0].edge[i] = NONE;
		for (j = 0; j < 3; j++) {
			ctx.bcU[j].edge[i] = NONE;
		}
	}
	for (i = 0; i < 8; i++) {
		ctx.bcV[0].vertex[i] = NONE;
		for (j = 0; j < 3; j++) {
			ctx.bcU[j].vertex[i] = NONE;
		}
	}
	ierr = DMDAVecRestoreArray(ctx.daScal,fields.VIrrev,&v_array);CHKERRQ(ierr);
	ierr = DMDAVecRestoreArrayDOF(ctx.daVect,ctx.coordinates,&coords_array);CHKERRQ(ierr);
	ctx.hasCrackPressure = PETSC_FALSE;
	ierr = VecCopy(fields.VIrrev,fields.V);CHKERRQ(ierr);
	for(ctx.timestep = 1; ctx.timestep < ctx.maxtimestep; ctx.timestep++){	
		ierr = PetscPrintf(PETSC_COMM_WORLD,"Time step %i\n",ctx.timestep);CHKERRQ(ierr);
		ierr = VecSet(fields.BCU,0.0);CHKERRQ(ierr);
		ierr = DMDAVecGetArrayDOF(ctx.daVect,fields.BCU,&bcu_array);CHKERRQ(ierr);
		switch (orientation) {
			case 1:
				ierr                = PetscPrintf(PETSC_COMM_WORLD,"Applying normal displacement boundary condition on face Z0 to simulate tension \n");CHKERRQ(ierr);
				ctx.bcU[0].face[X0] = ZERO;ctx.bcU[0].face[X1] = ZERO;
				ctx.bcU[2].face[X0] = ZERO;ctx.bcU[2].face[X1] = ZERO;
				
				ctx.bcU[0].face[Y0] = ZERO;ctx.bcU[0].face[Y1] = ZERO;
				ctx.bcU[2].face[Y0] = ZERO;ctx.bcU[2].face[Y1] = ZERO;
				
				ctx.bcU[0].face[Z0] = ZERO;ctx.bcU[0].face[Z1] = ZERO;
				ctx.bcU[1].face[Z0] = FIXED;ctx.bcU[1].face[Z1] = ZERO;
				ctx.bcU[2].face[Z0] = ZERO;ctx.bcU[2].face[Z1] = ZERO;
				
				for (k = zs; k < zs+zm; k++) {
					for (j = ys; j < ys+ym; j++) {
						for (i = xs; i < xs+xm; i++) {
							if (k == 0 && j > ny/2) {
								bcu_array[k][j][i][1] = ctx.timestep*bc;
							}
							if (k == 0 && j <= ny/2) {
								bcu_array[k][j][i][1] = -ctx.timestep*bc;
							}
						}
					}
				}
				break;
			case 2:
				ierr                = PetscPrintf(PETSC_COMM_WORLD,"Applying tangential displacement boundary condition on face Z0 to simulate in-plane shear\n");CHKERRQ(ierr);				
				ctx.bcU[0].face[X0] = ZERO;ctx.bcU[0].face[X1] = ZERO;
				ctx.bcU[1].face[X0] = ZERO;ctx.bcU[1].face[X1] = ZERO;
				
				ctx.bcU[0].face[Y0] = ZERO;ctx.bcU[0].face[Y1] = ZERO;
				ctx.bcU[1].face[Y0] = ZERO;ctx.bcU[1].face[Y1] = ZERO;
				
				ctx.bcU[0].face[Z0] = ZERO;ctx.bcU[0].face[Z1] = ZERO;
				ctx.bcU[1].face[Z0] = ZERO;ctx.bcU[1].face[Z1] = ZERO;
				ctx.bcU[2].face[Z0] = FIXED;ctx.bcU[2].face[Z1] = ZERO;
				
				for (k = zs; k < zs+zm; k++) {
					for (j = ys; j < ys+ym; j++) {
						for (i = xs; i < xs+xm; i++) {
							if (k == 0 && j > ny/2) {
								bcu_array[k][j][i][2] = ctx.timestep*bc;
							}
							if (k == 0 && j <= ny/2) {
								bcu_array[k][j][i][2] = -ctx.timestep*bc;
							}
						}
					}
				}
				break;
			case 3:
				ierr                = PetscPrintf(PETSC_COMM_WORLD,"Applying tangential displacement boundary condition on face Z0 to simulate out-of-plane shear\n");CHKERRQ(ierr);				
				ctx.bcU[1].face[X0] = ZERO;ctx.bcU[1].face[X1] = ZERO;
				ctx.bcU[2].face[X0] = ZERO;ctx.bcU[2].face[X1] = ZERO;
				
				ctx.bcU[1].face[Y0] = ZERO;ctx.bcU[1].face[Y1] = ZERO;
				ctx.bcU[2].face[Y0] = ZERO;ctx.bcU[2].face[Y1] = ZERO;
				
				ctx.bcU[0].face[Z0] = FIXED;ctx.bcU[0].face[Z1] = ZERO;
				ctx.bcU[1].face[Z0] = ZERO;ctx.bcU[1].face[Z1] = ZERO;
				ctx.bcU[2].face[Z0] = ZERO;ctx.bcU[2].face[Z1] = ZERO;
				
				for (k = zs; k < zs+zm; k++) {
					for (j = ys; j < ys+ym; j++) {
						for (i = xs; i < xs+xm; i++) {
							if (k == 0 && j > ny/2) {
								bcu_array[k][j][i][0] = ctx.timestep*bc;
							}
							if (k == 0 && j <= ny/2) {
								bcu_array[k][j][i][0] = -ctx.timestep*bc;
							}
						}
					}
				}
				break;
			default:
				SETERRQ1(PETSC_COMM_WORLD,PETSC_ERR_USER,"ERROR: Orientation should be between 1 & 3, got %i\n",orientation);
				break;
		}
		ierr = DMDAVecRestoreArrayDOF(ctx.daVect,fields.BCU,&bcu_array);CHKERRQ(ierr);
		ierr = VFTimeStepPrepare(&ctx,&fields);CHKERRQ(ierr);
		do {
			ierr = PetscPrintf(PETSC_COMM_WORLD,"alt min step %i\n",altminit);CHKERRQ(ierr);
			ierr = VF_StepU(&fields,&ctx);CHKERRQ(ierr);
			ierr = VecCopy(fields.V,Vold);CHKERRQ(ierr);
			ierr = VF_StepV(&fields,&ctx);CHKERRQ(ierr);
			ierr = VecCopy(fields.V,fields.VIrrev);CHKERRQ(ierr);

			ierr = VecAXPY(Vold,-1.,fields.V);CHKERRQ(ierr);
			ierr = VecNorm(Vold,NORM_INFINITY,&errV);CHKERRQ(ierr);
			ierr = PetscPrintf(PETSC_COMM_WORLD,"   Max. change on V: %e\n",errV);CHKERRQ(ierr);
			altminit++;
		} while (errV > ctx.altmintol && altminit <= ctx.altminmaxit);
		ctx.ElasticEnergy = 0;
		ctx.InsituWork    = 0;
		ctx.PressureWork  = 0.;
		ierr              = VF_UEnergy3D(&ctx.ElasticEnergy,&ctx.InsituWork,&ctx.PressureWork,&fields,&ctx);CHKERRQ(ierr);
		ierr              = VF_VEnergy3D(&ctx.SurfaceEnergy,&fields,&ctx);CHKERRQ(ierr);
		ctx.TotalEnergy = ctx.ElasticEnergy-ctx.InsituWork-ctx.PressureWork;
		ierr = PetscPrintf(PETSC_COMM_WORLD,"Surface energy:            %e\n",ctx.SurfaceEnergy);CHKERRQ(ierr);
		ierr = PetscPrintf(PETSC_COMM_WORLD,"Elastic Energy:            %e\n",ctx.ElasticEnergy);CHKERRQ(ierr);
		if (ctx.hasCrackPressure) {
			ierr = PetscPrintf(PETSC_COMM_WORLD,"Work of pressure forces:   %e\n",ctx.PressureWork);CHKERRQ(ierr);
		}
		if (ctx.hasInsitu) {
			ierr = PetscPrintf(PETSC_COMM_WORLD,"Work of surface forces:    %e\n",ctx.InsituWork);CHKERRQ(ierr);
		}
		ierr = PetscPrintf(PETSC_COMM_WORLD,"Total Mechanical energy:  %e\n",ctx.ElasticEnergy-InsituWork-ctx.PressureWork);CHKERRQ(ierr);
		ierr = VolumetricCrackOpening(&ctx.CrackVolume, &ctx, &fields);CHKERRQ(ierr);   
		switch (ctx.fileformat) {
			case FILEFORMAT_HDF5:
				ierr = FieldsH5Write(&ctx,&fields);
				break;
			case FILEFORMAT_BIN:
				ierr = FieldsBinaryWrite(&ctx,&fields);
				break;
		}
		altminit = 1;
		ierr = PetscPrintf(PETSC_COMM_WORLD,"###################################################################\n\n\n");CHKERRQ(ierr);
		ierr = PetscPrintf(PETSC_COMM_WORLD,"#        VF crack volume change = %e\t      \n", ctx.CrackVolume);CHKERRQ(ierr);
	}
	ierr = VFFinalize(&ctx,&fields);CHKERRQ(ierr);
	ierr = PetscFinalize();
	return(0);
}

