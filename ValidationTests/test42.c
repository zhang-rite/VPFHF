/*
 test42.c: 1D SNES. Heat problem with heat source term = 1. All temperature boundary condition. [T(0) = 1; T(L) = 0. temperature boundaries.
 (c) 2010-2013 Chukwudi Chukwudozie cchukw1@tigers.lsu.edu
 
 ./test42 -n 51,5,2 -l 1,0.4,0.01  -theta 1  -timestepsize 0.01 -maxtimestep 100 -cond 1
 ./test42 -n 51,5,2 -l 1,0.4,0.01  -theta 1  -timestepsize 0.05 -maxtimestep 150 -cond 0.1 
 ./test42 -n 51,5,2 -l 1,0.4,0.01  -theta 1  -timestepsize 0.05 -maxtimestep 150 -cond 0.01
 
 */

#include "petsc.h"
#include "VFCartFE.h"
#include "VFCommon.h"
#include "VFMech.h"
#include "VFFlow.h"
#include "VFHeat.h"

VFCtx               ctx;
VFFields            fields;

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{	
	PetscErrorCode  ierr;
	PetscViewer		viewer;
	PetscViewer     logviewer;
	char			filename[FILENAME_MAX];
	PetscInt		i,j,k,c,nx,ny,nz,xs,xm,ys,ym,zs,zm,xs1,xm1,ys1,ym1,zs1,zm1;
	PetscReal		BBmin[3],BBmax[3];
	PetscReal		****coords_array;
	PetscReal		hx,hy,hz;
	PetscReal		lx,ly,lz;
	PetscReal		***heatsrc_array;
	PetscReal		****heatfluxbc_array;
	PetscReal		****cond_array;
	PetscReal		***Tbc_array;
	PetscReal		****vel_array;
	PetscReal		ux = 1.,condctvty = 1.;
	
	ierr = PetscInitialize(&argc,&argv,(char*)0,banner);CHKERRQ(ierr);
//	ctx.flowsolver = FLOWSOLVER_SNESMIXEDFEM;
//	ctx.flowsolver = HEATSOLVER_SNESFEM;
	ierr = VFInitialize(&ctx,&fields);CHKERRQ(ierr);
	ierr = DMDAGetInfo(ctx.daScal,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
					   NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
	ierr = DMDAGetCorners(ctx.daScal,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
	ierr = DMDAGetBoundingBox(ctx.daVect,BBmin,BBmax);CHKERRQ(ierr);
	ierr = VecSet(fields.velocity,0.);CHKERRQ(ierr);
	ierr = VecSet(fields.theta,0.);CHKERRQ(ierr);
	ierr = VecSet(ctx.Cond,0.);CHKERRQ(ierr);
	ierr = VecSet(ctx.HeatSource,0.);CHKERRQ(ierr);
	ierr = VecSet(ctx.TBCArray,4.);CHKERRQ(ierr);
	ierr = VecSet(ctx.prevT,0.);CHKERRQ(ierr);
	ierr = VecSet(ctx.PresBCArray,0.);CHKERRQ(ierr);
	ierr = DMDAVecGetArrayDOF(ctx.daVect,ctx.coordinates,&coords_array);CHKERRQ(ierr);	
	ierr = DMDAVecGetArray(ctx.daScal,ctx.HeatSource,&heatsrc_array);CHKERRQ(ierr);
	ierr = DMDAVecGetArrayDOF(ctx.daVect,ctx.HeatFluxBCArray,&heatfluxbc_array);CHKERRQ(ierr);
	ierr = DMDAVecGetArrayDOF(ctx.daVFperm,ctx.Cond,&cond_array);CHKERRQ(ierr);
	ierr = DMDAVecGetArray(ctx.daScal,ctx.TBCArray,&Tbc_array);CHKERRQ(ierr);
	ierr = DMDAGetCorners(ctx.daVFperm,&xs1,&ys1,&zs1,&xm1,&ym1,&zm1);CHKERRQ(ierr);
	ierr = DMDAVecGetArrayDOF(ctx.daVect,fields.velocity,&vel_array);CHKERRQ(ierr);
	ctx.hasHeatSources = PETSC_TRUE;


	lz = BBmax[2]-BBmin[2];
	ly = BBmax[1]-BBmin[1];
	lx = BBmax[0]-BBmin[0];
	hx = lx/(nx-1);
	hy = ly/(nx-1);
	hz = lz/(nz-1);
  ctx.matprop[0].rho = 0;
	ctx.matprop[0].Cp = 0;

  
  ierr = PetscOptionsGetReal(NULL,NULL,"-cond",&condctvty,NULL);CHKERRQ(ierr);

  
	for (k = zs1; k < zs1+zm1; k++) {
		for (j = ys1; j < ys1+ym1; j++) {
			for (i = xs1; i < xs1+xm1; i++) {
				cond_array[k][j][i][0] = condctvty;
			}
		}
	}
	for (i = 0; i < 6; i++) {
		ctx.bcT[0].face[i] = NONE;
    for(j = 0; j < 3; j++){
      ctx.bcQT[j].face[i] = NONE;
    }
	}
	for (i = 0; i < 12; i++) {
		ctx.bcT[0].edge[i] = NONE;
    for(j = 0; j < 3; j++){
      ctx.bcQT[j].edge[i] = NONE;
    }
	}
	for (i = 0; i < 8; i++) {
		ctx.bcT[0].vertex[i] = NONE;
    for(j = 0; j < 3; j++){
      ctx.bcQT[j].vertex[i] = NONE;
    }
	}
	ctx.bcT[0].face[X0] = FIXED;
	ctx.bcT[0].face[X1] = FIXED;
	for (k = zs; k < zs+zm; k++) {
		for (j = ys; j < ys+ym; j++) {
			for (i = xs; i < xs+xm; i++) {
				if(i == 0){
					Tbc_array[k][j][i] = 1.;
				}
				if(i == nx-1){
					Tbc_array[k][j][i] = 0.;
				}
			}
		}
	}	
	for (k = zs; k < zs+zm; k++) {
		for (j = ys; j < ys+ym; j++) {
			for (i = xs; i < xs+xm; i++) {
				heatsrc_array[k][j][i] = 0.; 
				vel_array[k][j][i][0] = ux;
			}
		}
	}
	ierr = DMDAVecRestoreArrayDOF(ctx.daVect,fields.velocity,&vel_array);CHKERRQ(ierr);	
	ierr = DMDAVecRestoreArray(ctx.daScal,ctx.TBCArray,&Tbc_array);CHKERRQ(ierr);
	ierr = DMDAVecRestoreArrayDOF(ctx.daVFperm,ctx.Cond,&cond_array);CHKERRQ(ierr);
	ierr = DMDAVecRestoreArrayDOF(ctx.daVect,ctx.HeatFluxBCArray,&heatfluxbc_array);CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(ctx.daScal,ctx.HeatSource,&heatsrc_array);CHKERRQ(ierr);
	ierr = DMDAVecRestoreArrayDOF(ctx.daVect,ctx.coordinates,&coords_array);CHKERRQ(ierr);
	for (ctx.timestep = 0; ctx.timestep < ctx.maxtimestep; ctx.timestep++){
		ierr = PetscPrintf(PETSC_COMM_WORLD,"\n\nProcessing step %i.\n",ctx.timestep);CHKERRQ(ierr);
		ctx.timevalue = ctx.timestep * ctx.maxtimevalue / (ctx.maxtimestep-1.);
		ierr = PetscPrintf(PETSC_COMM_WORLD,"\n\ntime value %f \n",ctx.timevalue);CHKERRQ(ierr);
    
		ierr = VF_HeatTimeStep(&ctx,&fields);CHKERRQ(ierr);
    
    ierr = VecCopy(ctx.RHST,ctx.RHSTpre);CHKERRQ(ierr);
    ierr = VecCopy(fields.theta,ctx.prevT);CHKERRQ(ierr);

        ierr = FieldsVTKWrite(&ctx,&fields,NULL,NULL);CHKERRQ(ierr);
		ierr = PetscSNPrintf(filename,FILENAME_MAX,"%s.log",ctx.prefix);CHKERRQ(ierr);
		ierr = PetscViewerASCIIOpen(PETSC_COMM_WORLD,filename,&logviewer);CHKERRQ(ierr);
		ierr = PetscLogView(logviewer);CHKERRQ(ierr);
		ierr = PetscViewerDestroy(&logviewer);
	}

	ctx.timestep++; 
	ierr = DMDAVecGetArray(ctx.daScal,fields.theta,&Tbc_array);CHKERRQ(ierr);
	for (k = zs; k < zs+zm; k++) {
		for (j = ys; j < ys+ym; j++) {
			for (i = xs; i < xs+xm; i++) {
				Tbc_array[k][j][i] = ( exp(ux*i*hx/condctvty)-exp(ux*lx/condctvty) )/(1.-exp(ux*lx/condctvty)); 
			}
		}
	}
	ierr = DMDAVecRestoreArray(ctx.daScal,fields.theta,&Tbc_array);CHKERRQ(ierr);
    ierr = FieldsVTKWrite(&ctx,&fields,NULL,NULL);CHKERRQ(ierr);
	ierr = VFFinalize(&ctx,&fields);CHKERRQ(ierr);
	ierr = PetscFinalize();
	return(0);
}

