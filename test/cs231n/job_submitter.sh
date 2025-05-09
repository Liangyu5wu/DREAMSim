#!/bin/bash
#
#SBATCH --account=atlas:default
#SBATCH --partition=roma
#SBATCH --job-name=test_job
#SBATCH --output=output_test-%j.txt
#SBATCH --error=error_test-%j.txt
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=4g
#SBATCH --time=10:00:00

unset KRB5CCNAME
export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
export ALRB_CONT_CMDOPTS="-B /sdf,/fs,/lscratch"
export ALRB_CONT_RUNPAYLOAD="source /sdf/data/atlas/u/liangyu/vertextiming/user.scheong.mc21_14TeV.601229.PhPy8EG_A14_ttbar_hdamp258p75_SingleLep.SuperNtuple.e8514_s4345_r15583.20250219_Output/myJobPayload.sh"

source $ATLAS_LOCAL_ROOT_BASE/user/atlasLocalSetup.sh -c el9 –pwd $PWD
