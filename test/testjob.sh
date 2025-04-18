#!/bin/bash

source /sdf/data/atlas/u/liangyu/dSiPM/DREAMSim/setup.sh

SRC_DIR="/sdf/data/atlas/u/liangyu/dSiPM/DREAMSim/sim/src"
OUTER_DIR="/sdf/data/atlas/u/liangyu/dSiPM/DREAMSim/test"
BUILD_DIR="/sdf/data/atlas/u/liangyu/dSiPM/DREAMSim/sim"
STEPPING_FILE="B4bSteppingAction.cc"
LUSTRE_DIR="/fs/ddn/sdf/group/atlas/d/liangyu/dSiPM"
RESULTS_FILE="${OUTER_DIR}/simulation_results_new_4.txt"

echo "Rod Layer Energy TotalEntries_Mean TotalEntries_RMS Above90Entries_Mean Above90Entries_RMS nOPs_Sum" > ${RESULTS_FILE}

echo "Starting scanning..."

TOTAL_EVENTS=2000
EVENTS_PER_JOB=20
NUM_JOBS=100
GUN_ENERGY_MIN=100
GUN_ENERGY_MAX=100
PARTICLE_NAME="pi-"


for a in $(seq 0 5 80); do
  for b in $(seq 0 5 80); do
  
    if ([ $a -lt 20 ] || ([ $a -eq 20 ] && [ $b -le 65 ])); then
      continue
    fi

    echo "========================================="
    echo "Setting rodNumber = $a, layerNumber = $b"
    
    cd $SRC_DIR
    sed -i "822s/rodNumber != [0-9]* || layerNumber != [0-9]*/rodNumber != $a || layerNumber != $b/" $STEPPING_FILE
    echo "$STEPPING_FILE 822 line gets modified!"

    TEMP_SCRIPT=$(mktemp)
    cat > $TEMP_SCRIPT << EOF
#!/bin/bash
rm -rf /sdf/data/atlas/u/liangyu/dSiPM/DREAMSim/sim/build
source /sdf/data/atlas/u/liangyu/dSiPM/DREAMSim/setup9.sh
cd $BUILD_DIR
mkdir build
cd build
cmake ..
make -j 4
EOF
    chmod +x $TEMP_SCRIPT
    singularity exec --bind=/cvmfs,/sdf,/fs,/lscratch /cvmfs/atlas.cern.ch/repo/containers/fs/singularity/x86_64-almalinux9 $TEMP_SCRIPT
    rm $TEMP_SCRIPT
    
    echo "rodNumber=$a, layerNumber=$b compiling completed!"
    echo "========================================="

    cd $OUTER_DIR
    WORK_DIR="rod${a}_layer${b}_E${GUN_ENERGY_MIN}_jobs"
    mkdir -p $WORK_DIR
    cd $WORK_DIR


    for ((job_id=1; job_id<=NUM_JOBS; job_id++)); do
      JOB_SCRIPT="job_rod${a}_layer${b}_E${GUN_ENERGY_MIN}_${job_id}.sh"
      PAYLOAD_SCRIPT="myJobPayload_rod${a}_layer${b}_E${GUN_ENERGY_MIN}_job${job_id}.sh"

      cat > $PAYLOAD_SCRIPT << EOF
#!/bin/bash
source /sdf/data/atlas/u/liangyu/dSiPM/DREAMSim/setup9.sh
cd /sdf/data/atlas/u/liangyu/dSiPM/DREAMSim/sim/build
./exampleB4b -b paramBatch03_single.mac -jobName ${PARTICLE_NAME}_job_rod${a}_layer${b} -runNumber ${a} -runSeq ${job_id} -numberOfEvents ${EVENTS_PER_JOB} -eventsInNtupe 100 -gun_particle ${PARTICLE_NAME} -gun_energy_min ${GUN_ENERGY_MIN} -gun_energy_max ${GUN_ENERGY_MAX} -sipmType 1
echo "Job ${job_id} for rod=${a}, layer=${b}, energy=${GUN_ENERGY_MIN}GeV completed!"
EOF
  
      
      cat > $JOB_SCRIPT << EOF
#!/bin/bash
#
#SBATCH --account=atlas:default
#SBATCH --partition=roma
#SBATCH --job-name=R${a}L${b}E${GUN_ENERGY_MIN}_${job_id}
#SBATCH --output=output_rod${a}_layer${b}_E${GUN_ENERGY_MIN}_job${job_id}-%j.txt
#SBATCH --error=error_rod${a}_layer${b}_E${GUN_ENERGY_MIN}_job${job_id}-%j.txt
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=4g
#SBATCH --time=2:00:00

unset KRB5CCNAME
export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
export ALRB_CONT_CMDOPTS="-B /sdf,/fs,/lscratch"
export ALRB_CONT_RUNPAYLOAD="source /sdf/data/atlas/u/liangyu/dSiPM/DREAMSim/test/${WORK_DIR}/${PAYLOAD_SCRIPT}"

source $ATLAS_LOCAL_ROOT_BASE/user/atlasLocalSetup.sh -c el9 –pwd $PWD
EOF

      chmod +x $JOB_SCRIPT
      chmod +x $PAYLOAD_SCRIPT
      sbatch $JOB_SCRIPT
      
      echo "Submitted job ${job_id} for rodNumber=${a}, layerNumber=${b}"
      
      if (( job_id % 50 == 0 )); then
        echo "Pausing for 3 seconds to prevent overwhelming the scheduler..."
        sleep 3
      fi
    done
    
    echo "All ${NUM_JOBS} jobs submitted for rodNumber=${a}, layerNumber=${b}"
    echo "========================================="

    echo "Waiting for all jobs for rodNumber=${a}, layerNumber=${b} to complete..."
    
    while true; do
      JOB_PREFIX="R${a}L${b}E${GUN_ENERGY_MIN}"
      RUNNING_JOBS=$(squeue -u $USER -h -o "%.15j" | grep "${JOB_PREFIX}_" | wc -l)
      
      if [ $RUNNING_JOBS -eq 0 ]; then
        echo "All jobs for $JOB_PREFIX have completed. Continuing with the next parameter set."
        break
      else
        echo "$(date): Still waiting for $RUNNING_JOBS jobs with prefix $JOB_PREFIX to complete..."
        sleep 15
      fi
    done

    cat > calculate_stats.C << EOF
void calculate_stats() {
  vector<double> total_entries;
  vector<double> above90_entries;
  vector<double> nOPs_entries;
  
  for (int job_id = 1; job_id <= ${NUM_JOBS}; job_id++) {
    TString filename = Form("/sdf/data/atlas/u/liangyu/dSiPM/DREAMSim/sim/build/mc_${PARTICLE_NAME}_job_rod${a}_layer${b}_run${a}_%d_Test_${EVENTS_PER_JOB}evt_${PARTICLE_NAME}_${GUN_ENERGY_MIN}_${GUN_ENERGY_MAX}.root", job_id);
    
    if (gSystem->AccessPathName(filename)) continue;
    
    TFile *f = TFile::Open(filename);
    if (!f || f->IsZombie()) {
        cout << "Warning: Unable to open file: " << filename << endl;
        if (f) {
            f->Close();
            delete f;
        }
        continue;
    }
    
    TTree *tree = nullptr;
    tree = (TTree*)f->Get("tree;4");
    if (tree) {
        cout << "Using tree;4 for file: " << filename << endl;
    } else {
        tree = (TTree*)f->Get("tree;3");
        if (tree) {
            cout << "Using tree;3 for file: " << filename << endl;
        } else {
            tree = (TTree*)f->Get("tree;2");
            if (tree) {
                cout << "Using tree;2 for file: " << filename << endl;
            } else {
                tree = (TTree*)f->Get("tree;1");
                if (tree) {
                    cout << "Using tree;1 for file: " << filename << endl;
                } else {
                    cout << "Warning: No tree object (tree;1 to tree;4) found in file: " << filename << endl;
                    f->Close();
                    continue;
                }
            }
        }
    }
    
    int n_total = tree->Draw("OP_pos_final_z", "", "goff");
    total_entries.push_back(n_total);
    
    int n_above90 = tree->Draw("OP_pos_final_z", "OP_pos_final_z > 90", "goff");
    above90_entries.push_back(n_above90);

    int n_nOPs = 0;
    if (tree->GetBranch("nOPs")) {
      n_nOPs = tree->Draw("nOPs", "", "goff");
    } else {
      cout << "Warning: nOPs branch not found in tree for file: " << filename << endl;
    }
    nOPs_entries.push_back(n_nOPs);
    f->Close();
  }
  
  double total_mean = 0, total_rms = 0;
  double above90_mean = 0, above90_rms = 0;
  long long nOPs_sum = 0; 
  
  for (size_t i = 0; i < total_entries.size(); i++) {
    total_mean += total_entries[i];
    above90_mean += above90_entries[i];
    nOPs_sum += nOPs_entries[i];
  }
  
  if (total_entries.size() > 0) {
    total_mean /= total_entries.size();
    above90_mean /= above90_entries.size();
    
    for (size_t i = 0; i < total_entries.size(); i++) {
      total_rms += (total_entries[i] - total_mean) * (total_entries[i] - total_mean);
      above90_rms += (above90_entries[i] - above90_mean) * (above90_entries[i] - above90_mean);
    }
    
    total_rms = sqrt(total_rms / total_entries.size());
    above90_rms = sqrt(above90_rms / above90_entries.size());
  }
  

  ofstream outfile("${OUTER_DIR}/stats_rod${a}_layer${b}_E${GUN_ENERGY_MIN}.txt");
  outfile << "Rod: ${a}" << endl;
  outfile << "Layer: ${b}" << endl;
  outfile << "Energy: ${GUN_ENERGY_MIN}" << endl;
  outfile << "Number of files processed: " << total_entries.size() << endl;
  outfile << "Total entries mean: " << total_mean << endl;
  outfile << "Total entries RMS: " << total_rms << endl;
  outfile << "Above 90 entries mean: " << above90_mean << endl;
  outfile << "Above 90 entries RMS: " << above90_rms << endl;
  outfile << "nOPs entries sum: " << nOPs_sum << endl;
  outfile.close();
  
  ofstream summary("${RESULTS_FILE}", ios::app);
  summary << "${a} ${b} ${GUN_ENERGY_MIN} " << total_mean << " " << total_rms << " " << above90_mean << " " << above90_rms << " " << nOPs_sum << endl;
  summary.close();
  
  cout << "Statistics calculated and saved." << endl;
  cout << "Rod: ${a}, Layer: ${b}, Energy: ${GUN_ENERGY_MIN}" << endl;
  cout << "Total entries mean: " << total_mean << ", RMS: " << total_rms << endl;
  cout << "Above 90 entries mean: " << above90_mean << ", RMS: " << above90_rms << endl;
  cout << "nOPs entries sum: " << nOPs_sum << endl;
}
EOF

    root -l -q -b calculate_stats.C
    rm calculate_stats.C
    echo "Statistics calculated for rodNumber=${a}, layerNumber=${b}"
    rm -rf $OUTER_DIR/$WORK_DIR
    
    echo "Moving to the next parameter set..."
  done
done

echo "Scanning and job submission completed!"
