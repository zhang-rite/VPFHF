/*
 test21.c: Solves for the displacement and v-field in arbitrary number of volume loaded penny cracks in 3d
 (c) 2010-2012 Chukwudi Chukwudozie cchukw1@tigers.lsu.edu
 */

#include "petsc.h"
#include "VFCartFE.h"
#include "VFCommon.h"
#include "VFMech.h"
#include "VFPermfield.h"

VFCtx               ctx;
VFFields            fields;

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
	PetscErrorCode  ierr;
	PetscViewer			viewer;
	PetscViewer     logviewer;
	PetscInt        orientation=1;
	PetscInt        i,j,c,nx,ny,nz,xs,xm,ys,ym,zs,zm;
	PetscReal		****coords_array;
	PetscReal		 ***v_array;  
	PetscReal       BBmin[3],BBmax[3];
	char            filename[FILENAME_MAX];
	PetscReal       p;
	PetscReal       p_old;
	PetscReal       p_epsilon = 1.e-5;
	PetscInt	    altminit=1;
	Vec			    Vold;
	PetscReal	    errV=1e+10;
	PetscReal	    lx,ly,lz;
	PetscReal	    q,maxvol = .3;
	VFPennyCrack	*crack;
	PetscInt		nc = 0;
	char			prefix[PETSC_MAX_PATH_LEN+1];
	Vec				V;

	ierr = PetscInitialize(&argc,&argv,(char*)0,banner);CHKERRQ(ierr);
	ierr = VFInitialize(&ctx,&fields);CHKERRQ(ierr);
	ierr = PetscOptionsGetInt(NULL,"-nc",&nc,NULL);CHKERRQ(ierr);
	ierr = PetscOptionsGetInt(NULL,"-orientation",&orientation,NULL);CHKERRQ(ierr);
	ierr = DMDAGetInfo(ctx.daScal,NULL,&nx,&ny,&nz,NULL,NULL,NULL,
					   NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
	ierr = DMDAGetCorners(ctx.daScal,&xs,&ys,&zs,&xm,&ym,&zm);CHKERRQ(ierr);
	ierr = DMDAGetBoundingBox(ctx.daScal,BBmin,BBmax);CHKERRQ(ierr);	
	ierr = PetscOptionsGetReal(NULL,"-maxvol",&maxvol,NULL);CHKERRQ(ierr);
	ctx.maxtimestep = 150;
	ierr = PetscOptionsGetInt(NULL,"-maxtimestep",&ctx.maxtimestep,NULL);CHKERRQ(ierr);
	q = maxvol / ctx.maxtimestep;
	
	ierr = DMDAVecGetArrayDOF(ctx.daVect,ctx.coordinates,&coords_array);CHKERRQ(ierr);
	ierr = DMCreateGlobalVector(ctx.daScal,&V);CHKERRQ(ierr);
	ierr = VecSet(V,1.0);CHKERRQ(ierr);
	ierr = VecSet(fields.VIrrev,1.0);CHKERRQ(ierr);
	ierr = VecSet(fields.V,1.0);CHKERRQ(ierr);
	ierr = DMDAVecGetArray(ctx.daScal,fields.VIrrev,&v_array);CHKERRQ(ierr);    
	lz = BBmax[2];
	ly = BBmax[1];
	lx = BBmax[0];	
	/*
	 Reset all BC for U and V
	 */
	for (i = 0; i < 6; i++) {
		ctx.bcV[0].face[i]=NONE;
		for (j = 0; j < 3; j++) {
			ctx.bcU[j].face[i] = NONE;
		}
	}
	for (i = 0; i < 12; i++) {
		ctx.bcV[0].edge[i]=NONE;
		for (j = 0; j < 3; j++) {
			ctx.bcU[j].edge[i] = NONE;
		}
	}
	for (i = 0; i < 8; i++) {
		ctx.bcV[0].vertex[i]=NONE;
		for (j = 0; j < 3; j++) {
			ctx.bcU[j].vertex[i] = NONE;
		}
	}
	/*Initializing the v-field*/	
	ierr = PetscMalloc(nc*sizeof(VFPennyCrack),&crack);CHKERRQ(ierr);
	for (i = 0; i < nc; i++) {
		ierr = PetscSNPrintf(prefix,PETSC_MAX_PATH_LEN,"c%d_",i);CHKERRQ(ierr);
		ierr = VFPennyCrackCreate(&crack[i]);CHKERRQ(ierr);
		ierr = VFPennyCrackGet(prefix,&crack[i]);CHKERRQ(ierr);
		ierr = VFPennyCrackView(&crack[i],PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
	}	
	for (c = 0; c < nc; c++) {
		ierr = VFPennyCrackBuildVAT2(V,&crack[c],&ctx);CHKERRQ(ierr);
		ierr = VecPointwiseMin(fields.V,V,fields.V);CHKERRQ(ierr);
	}
	ierr = VecDestroy(&V);CHKERRQ(ierr);
	ierr = VecCopy(fields.V,fields.VIrrev);CHKERRQ(ierr);
	switch (orientation) {
		case 1:
			ierr = PetscPrintf(PETSC_COMM_WORLD,"Buildin %g number of penny-shaped cracks of arbitrary dimensions\n",
							   nc);CHKERRQ(ierr);	  
			/*	face X0	*/
			ctx.bcU[0].face[X0]= ZERO;
			ctx.bcU[1].face[X0]= ZERO;
			ctx.bcU[2].face[X0]= ZERO;
			/*	face X1	*/
			ctx.bcU[0].face[X1]= ZERO;
			ctx.bcU[1].face[X1]= ZERO;
			ctx.bcU[2].face[X1]= ZERO;
			/*	face Y0	*/
			ctx.bcU[0].face[Y0]= ZERO;
			ctx.bcU[1].face[Y0]= ZERO;
			ctx.bcU[2].face[Y0]= ZERO;
			/*	face Y1	*/
			ctx.bcU[0].face[Y1]= ZERO;		  
			ctx.bcU[1].face[Y1]= ZERO;		  
			ctx.bcU[2].face[Y1]= ZERO;		  
			/*	face Z0	*/
			ctx.bcU[0].face[Z0]= ZERO;
			ctx.bcU[1].face[Z0]= ZERO;
			ctx.bcU[2].face[Z0]= ZERO;
			/*	face Z1	*/
			ctx.bcU[0].face[Z1]= ZERO;
			ctx.bcU[1].face[Z1]= ZERO;
			ctx.bcU[2].face[Z1]= ZERO;
			break;
		case 2:
			ierr = PetscPrintf(PETSC_COMM_WORLD,"Buildin %g number of penny-shaped cracks of arbitrary dimensions\n",
							   nc);CHKERRQ(ierr);	  
			/*	face X0	*/
			ctx.bcU[0].face[X0]= ZERO;
			/*	face X1	*/
			ctx.bcU[0].face[X1]= ZERO;
			/*	face Y0	*/
			ctx.bcU[0].face[Y0]= ZERO;
			ctx.bcU[1].face[Y0]= ZERO;
			ctx.bcU[2].face[Y0]= ZERO;
			/*	face Y1	*/
			ctx.bcU[0].face[Y1]= ZERO;		  
			ctx.bcU[1].face[Y1]= ZERO;		  
			ctx.bcU[2].face[Y1]= ZERO;		  
			/*	face Z0	*/
			ctx.bcU[2].face[Z0]= ZERO;
			/*	face Z1	*/
			ctx.bcU[2].face[Z1]= ZERO;
			break;
		case 3:
			ierr = PetscPrintf(PETSC_COMM_WORLD,"Buildin %g number of penny-shaped cracks of arbitrary dimensions\n",
							   nc);CHKERRQ(ierr);	  
			/*	face X0	*/
			ctx.bcU[1].face[X0]= ZERO;
			ctx.bcU[2].face[X0]= ZERO;
			/*	face X1	*/
			ctx.bcU[1].face[X1]= ZERO;
			ctx.bcU[2].face[X1]= ZERO;
			/*	face Y0	*/
			ctx.bcU[0].face[Y0]= ZERO;
			ctx.bcU[1].face[Y0]= ZERO;
			ctx.bcU[2].face[Y0]= ZERO;
			/*	face Y1	*/
			ctx.bcU[0].face[Y1]= ZERO;		  
			ctx.bcU[1].face[Y1]= ZERO;		  
			ctx.bcU[2].face[Y1]= ZERO;		  
			/*	face Z0	*/
			ctx.bcU[0].face[Z0]= ZERO;
			ctx.bcU[1].face[Z0]= ZERO;
			/*	face Z1	*/
			ctx.bcU[0].face[Z1]= ZERO;
			ctx.bcU[1].face[Z1]= ZERO;
			break;
		case 4:
			ierr = PetscPrintf(PETSC_COMM_WORLD,"Buildin %g number of penny-shaped cracks of arbitrary dimensions\n",
							   nc);CHKERRQ(ierr);	  
			/*	face X0	*/
			ctx.bcU[0].face[X0]= ZERO;
			/*	face X1	*/
			ctx.bcU[0].face[X1]= ZERO;
			/*	face Y0	*/
			ctx.bcU[1].face[Y0]= ZERO;
			/*	face Y1	*/
			ctx.bcU[1].face[Y1]= ZERO;		  
			/*	face Z0	*/
			ctx.bcU[2].face[Z0]= ZERO;
			/*	face Z1	*/
			ctx.bcU[2].face[Z1]= ZERO;
			break;
		case 5:
			ierr = PetscPrintf(PETSC_COMM_WORLD,"Buildin %g number of penny-shaped cracks of arbitrary dimensions\n",
							   nc);CHKERRQ(ierr);	  
			/*	face X0	*/
			ctx.bcU[1].face[X0]= ZERO;
			ctx.bcU[2].face[X0]= ZERO;
			/*	face X1	*/
			ctx.bcU[1].face[X1]= ZERO;
			ctx.bcU[2].face[X1]= ZERO;
			/*	face Y0	*/
			ctx.bcU[0].face[Y0]= ZERO;
			ctx.bcU[2].face[Y0]= ZERO;
			/*	face Y1	*/
			ctx.bcU[0].face[Y1]= ZERO;		  
			ctx.bcU[2].face[Y1]= ZERO;		  
			/*	face Z0	*/
			ctx.bcU[0].face[Z0]= ZERO;
			ctx.bcU[1].face[Z0]= ZERO;
			/*	face Z1	*/
			ctx.bcU[0].face[Z1]= ZERO;
			ctx.bcU[1].face[Z1]= ZERO;
			break;
		default:
			SETERRQ1(PETSC_COMM_WORLD,PETSC_ERR_USER,"ERROR: Orientation should be one of {1,2,3,4,5,6,7}, got %i\n",orientation);
			break;
	}  
	ierr = DMDAVecRestoreArray(ctx.daScal,fields.VIrrev,&v_array);CHKERRQ(ierr);
	ierr = DMDAVecRestoreArrayDOF(ctx.daVect,ctx.coordinates,&coords_array);CHKERRQ(ierr);
	ierr = VFTimeStepPrepare(&ctx,&fields);CHKERRQ(ierr);
	
	ctx.hasCrackPressure = PETSC_TRUE;
	ierr = VecDuplicate(fields.V,&Vold);CHKERRQ(ierr);

	ierr = PetscViewerCreate(PETSC_COMM_WORLD, &viewer);CHKERRQ(ierr);
	ierr = PetscViewerSetType(viewer, PETSCVIEWERASCII);CHKERRQ(ierr);
	ierr = PetscViewerFileSetMode(viewer, FILE_MODE_APPEND);CHKERRQ(ierr);
	ierr = PetscSNPrintf(filename,FILENAME_MAX,"%s.pres",ctx.prefix);CHKERRQ(ierr);
	ierr = PetscViewerFileSetName(viewer,filename);CHKERRQ(ierr);
	ierr = PetscViewerASCIIPrintf(viewer, "#Time step \t Volume \t Pressure \t SurfaceEnergy \t ElasticEnergy \t PressureForces \t TotalMechEnergy \n");CHKERRQ(ierr);
	p = 1.e-5;
	ierr = VecSet(fields.theta,0.0);CHKERRQ(ierr);
	ierr = VecSet(fields.thetaRef,0.0);CHKERRQ(ierr);
	ierr = VecSet(fields.pressure,p);CHKERRQ(ierr);
	ctx.timevalue = 0;
	ctx.maxtimestep = 100;
	for (ctx.timestep = 1; ctx.timestep < ctx.maxtimestep; ctx.timestep++){
		ierr = PetscPrintf(PETSC_COMM_WORLD,"Time step %i, injected volume %g\n",ctx.timestep,q*ctx.timestep);CHKERRQ(ierr);
		ierr = VecCopy(fields.V,fields.VIrrev);CHKERRQ(ierr); 
		do {
			p_old = p;
			ierr = PetscPrintf(PETSC_COMM_WORLD,"  Time step %i, alt min step %i with pressure %g\n",ctx.timestep,altminit, p);CHKERRQ(ierr);
			ierr = VF_StepU(&fields,&ctx);CHKERRQ(ierr);
			ierr = VecScale(fields.U,1./p);CHKERRQ(ierr);
			ierr = VolumetricCrackOpening(&ctx.CrackVolume, &ctx, &fields);CHKERRQ(ierr);   
			p = q*ctx.timestep/ctx.CrackVolume;
			ierr = VecCopy(fields.V,Vold);CHKERRQ(ierr);
			ierr = VecScale(fields.U,p);CHKERRQ(ierr);
			ierr = VecSet(fields.pressure,p);CHKERRQ(ierr);
			ierr = VF_StepV(&fields,&ctx);CHKERRQ(ierr);

			ierr = VecAXPY(Vold,-1.,fields.V);CHKERRQ(ierr);
			ierr = VecNorm(Vold,NORM_INFINITY,&errV);CHKERRQ(ierr);
			ierr = PetscPrintf(PETSC_COMM_WORLD,"    Max. change on V: %e\n",errV);CHKERRQ(ierr);
			ierr = PetscPrintf(PETSC_COMM_WORLD,"    Max. change on p: %e\n", PetscAbs(p-p_old));CHKERRQ(ierr);
			altminit++;
		} while (PetscAbs(p-p_old) >= p_epsilon && altminit <= ctx.altminmaxit);
		ierr = VolumetricCrackOpening(&ctx.CrackVolume, &ctx, &fields);CHKERRQ(ierr);   
		switch (ctx.fileformat) {
			case FILEFORMAT_HDF5:       
				ierr = FieldsH5Write(&ctx,&fields);CHKERRQ(ierr);
				break;
			case FILEFORMAT_BIN:
				ierr = FieldsBinaryWrite(&ctx,&fields);CHKERRQ(ierr);
				break; 
		}
		ierr = PetscSNPrintf(filename,FILENAME_MAX,"%s.log",ctx.prefix);CHKERRQ(ierr);
		ierr = PetscViewerASCIIOpen(PETSC_COMM_WORLD,filename,&logviewer);CHKERRQ(ierr);
		ierr = PetscLogView(logviewer);CHKERRQ(ierr);
		ierr = PetscViewerDestroy(&logviewer);CHKERRQ(ierr);
		ctx.ElasticEnergy=0;
		ctx.InsituWork=0;
		ctx.PressureWork = 0.;
		ierr = VF_UEnergy3D(&ctx.ElasticEnergy,&ctx.InsituWork,&ctx.PressureWork,&fields,&ctx);CHKERRQ(ierr);
		ierr = VF_VEnergy3D(&ctx.SurfaceEnergy,&fields,&ctx);CHKERRQ(ierr);
		ctx.TotalEnergy = ctx.ElasticEnergy - ctx.InsituWork - ctx.PressureWork + ctx.SurfaceEnergy;
		ierr = PetscViewerASCIIPrintf(viewer, "%d \t %e \t %e \t %e \t %e \t %e \t %e\n", ctx.timestep , ctx.CrackVolume, p, ctx.SurfaceEnergy, ctx.ElasticEnergy, ctx.PressureWork, ctx.TotalEnergy);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(ctx.energyviewer,"%i   \t%e   \t%e   \t%e   \t%e   \t%e\n",ctx.timestep,ctx.ElasticEnergy,
                                  ctx.InsituWork,ctx.SurfaceEnergy,ctx.PressureWork,ctx.TotalEnergy);CHKERRQ(ierr);
		altminit = 0.;
	}
	ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
	ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
	ierr = PetscViewerDestroy(&logviewer);CHKERRQ(ierr);
	ierr = VecDestroy(&Vold);CHKERRQ(ierr);

	ierr = VFFinalize(&ctx,&fields);CHKERRQ(ierr);
	ierr = PetscFinalize();
	return(0);
}

