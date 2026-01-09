#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
File name: script.py
Author: Filip Cerny
Created: 09.01.2026
Version: 1.0
Description: 
"""
import argparse
import datetime
import os
import random
import subprocess
import time
from dataclasses import dataclass
import pandas as pd
from tqdm import tqdm

# define constatnt

GSAT_PROB = 0.4

GSAT_EXE_FILEPATH = "bin/gsat2"

# define output data structure with data class
@dataclass
class AlgorithmOutput:
    iteration_count: int
    clause_satisfied: int
    clause_total: int

def run_gsat(cnf_filepath, seed, max_flips, max_tries,timeout_seconds=None, prob=GSAT_PROB):
    """ Run executable linux from bin/gsat2 with given CNF formula."""

    start = time.monotonic()
    try:

        # zavoláme gsat2 s parametry (-max_flips)
        cmd = ["./" + GSAT_EXE_FILEPATH,
               cnf_filepath,
               "-r", seed.__str__(), # random generator
               "-i", max_flips.__str__(), # max flips
               "-p", prob.__str__(), #probability
               "-T", max_tries.__str__(), # max tries
               ]
        #print(f"Running GSAT command: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout_seconds)

        equation_result = result.stdout
        output = result.stderr

        #parse output by space
        output_parsed = output.split()

        iteration_count = output_parsed[0]
        clause_satisfied = output_parsed[2]
        clause_total = output_parsed[3]

        # if exit code is not 0, return None
        if result.returncode != 0:
            raise Exception(f"GSAT failed with exit code {result.returncode}")


        return AlgorithmOutput(
            iteration_count=int(iteration_count),
            clause_satisfied=int(clause_satisfied),
            clause_total=int(clause_total)
        )
    except subprocess.TimeoutExpired:
        runtime = time.monotonic() - start
        print(f"GSAT timed out after {runtime} seconds.")
        return None


PROBSAT_EXE_FILEPATH = "bin/probSAT"

PROBSAT_CM = 0 # set defualt in ./bin/probSAT
PROBSAT_CB = 2.3
#  cmd = ["./probSAT", filename, "cb=2.3", "cm=0", f"cutoff={max_flips}"]

def run_probsat(cnf_filepath, seed, max_flips, max_tries, timeout_seconds=None, cb=PROBSAT_CB):
    """ Run executable linux from bin/probSAT with given CNF formula."""
    time_start = time.monotonic()
    try:

        if max_tries is not None:
            cmd = ["./" + PROBSAT_EXE_FILEPATH,
                   "--cb", cb.__str__(),
                   "-m", max_flips.__str__(),
                   "--runs", max_tries.__str__(),
                   cnf_filepath,
                   seed.__str__()
                   ]
        else:
            cmd = ["./" + PROBSAT_EXE_FILEPATH,
                   "--cb", cb.__str__(),
                   "-m", max_flips.__str__(),
                   cnf_filepath,
                   seed.__str__()
                   ]

        #print(f"Running probSAT command: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout_seconds)

        output = result.stdout
        #output = result.stderr

        if result.returncode != 10 and result.returncode != 0:
            raise Exception(f"probSAT failed with exit code {result.returncode}")

        # region PARSE OUTPUT

        #parse output by space
        max_clauses = None
        satisfied_clauses = None
        total_flips = 0
        # get number of clauses find after "c number of clauses"
        if "c number of clauses" in output:
            for line in output.splitlines():
                if line.startswith("c number of clauses"):
                    max_clauses = int(line.split()[5])

        for line in output.splitlines():
            if line.startswith("c numFlips"):
                number = line.split()[3]
                number = int(''.join(filter(str.isdigit, number)))
                # remove all non digit characters
                if number < max_flips:
                    total_flips = number


        # get if Satifiable find if in output is "s SATISFIABLE"
        if "s SATISFIABLE" in output and result.returncode == 10:
            # get number of flips from output line starting with "c flips"
            satisfied_clauses = max_clauses
        else:
            if result.returncode == 0:
                # parse all lines starting wiht c UNKOKNOWN
                satisfied_clauses = []
                if "c UNKNOWN" in output:
                    for line in output.splitlines():
                        if line.startswith("c UNKNOWN"):
                            total_flips += max_flips
                            line_parts = line.split()
                            number = line_parts[3]
                            # remove all non digit characters
                            num_satisfied = ''.join(filter(str.isdigit, number))
                            satisfied_clauses.append(int(num_satisfied))

                    # find min in num_satisfied
                    min_value = min(satisfied_clauses)
                    satisfied_clauses = max_clauses - min_value
            else:
                raise Exception(f"probSAT failed with exit code {result.returncode}")

        if total_flips is None or satisfied_clauses is None or max_clauses is None:
            raise Exception(f"probSAT output parsing failed.")
        # endregion

        return AlgorithmOutput(
            iteration_count=total_flips,
            clause_satisfied=satisfied_clauses,
            clause_total=max_clauses
        )
    except subprocess.TimeoutExpired:
        runtime = time.monotonic() - time_start
        print(f"probSAT timed out after {runtime} seconds.")
        return None



def run_experiment(repeat_for_instances, data_num_instances_per_n, alg_max_tries, alg_timeout_seconds):
    n_values = [20, 40, 60, 75]
    output_dir = "data/generated_instances"

    now = datetime.datetime.now()
    timestamp = now.strftime("%Y%m%d_%H%M%S")
    os.makedirs("results", exist_ok=True)
    result_filename = f"results/experiment_results_{timestamp}.csv"
    print(f"Results will be saved to {result_filename}")

    instances_filenames = []
    for inst_var_count in n_values:
        for inst_id in range(data_num_instances_per_n):
            cnf_filepath = os.path.join(output_dir, f"sat_n{inst_var_count}_inst{inst_id + 1}.cnf")
            instances_filenames.append(cnf_filepath)


    # status = widgets.Text(value='', description='Status:')
    # # make it wider
    # status.layout.width = '800px'
    # display(status)


    for idx, cnf_filepath in enumerate(tqdm(instances_filenames, desc="Processing instances", position=0)):
        #print(f"Processing instance file: {cnf_filepath}")

        # extract info from instance filename
        parts = os.path.basename(cnf_filepath).split("_")
        var_count_filename = int(parts[1][1:])  # extract n from 'sat_n
        inst_id = int(parts[2][4:-4])  # extract instance number from 'instX.cnf'

        # extract info from first line of CNF file
        with open(cnf_filepath, 'r') as f:
            for line in f:
                if line.startswith('p cnf'):
                    parts = line.strip().split()
                    file_var_count = int(parts[2])
                    file_clause_count = int(parts[3])
                    break
        if file_var_count != var_count_filename:
            raise ValueError(f"Variable count mismatch in file {cnf_filepath}: filename says {var_count_filename}, file says {file_var_count}")

        inst_var_count = file_var_count
        inst_clause_count = file_clause_count

        # ONE INSTANCE
        for repeat in tqdm(range(repeat_for_instances), desc=f"  Repeats for instance {inst_id} with variables {inst_var_count}", position=1, leave=False):

            status.value = f"Processing instance {idx + 1}/{len(instances_filenames)}: n={inst_var_count}, instance={inst_id}, repeat={repeat + 1}"

            # ALGORITHM PARAMETERS
            random_seed = random.randint(1, 1000000).__str__()
            max_flips = inst_var_count * 100
            max_tries = alg_max_tries
            filepath = cnf_filepath
            timeout_seconds = alg_timeout_seconds

            # Run GSAT
            gsat_output = run_gsat(filepath, seed=random_seed, max_flips=max_flips, max_tries=max_tries, timeout_seconds=timeout_seconds)
            if gsat_output is not None:
                if gsat_output.clause_total != inst_clause_count:
                    raise ValueError(f"Clause count mismatch in GSAT output for file {cnf_filepath}: expected {inst_clause_count}, got {gsat_output.clause_total}")

                row_gsat ={
                    "ALG_algorithm": "GSAT",
                    "ALG_max_flips": max_flips,
                    "ALG_max_tries": max_tries,
                    "ALG_seed": random_seed,
                    "INST_n_variables": inst_var_count,
                    "INST_instance_id": inst_id,
                    "INST_clause_total": inst_clause_count,
                    "OUT_clause_satisfied": gsat_output.clause_satisfied,
                    "OUT_flips_count": gsat_output.iteration_count,
                }
            else:
                # TIMEOUT
                row_gsat ={
                    "ALG_algorithm": "GSAT",
                    "ALG_max_flips": max_flips,
                    "ALG_max_tries": max_tries,
                    "ALG_seed": random_seed,
                    "INST_n_variables": inst_var_count,
                    "INST_instance_id": inst_id,
                    "INST_clause_total": inst_clause_count,
                    "OUT_clause_satisfied": None,
                    "OUT_flips_count": None,
                }
            pd.DataFrame([row_gsat]).to_csv(result_filename, mode="a", header=not os.path.exists(result_filename), index=False)
            # Run probSAT
            probsat_output = run_probsat(filepath, seed=random_seed, max_flips=max_flips, max_tries=max_tries,
                                         timeout_seconds=timeout_seconds)
            if probsat_output is not None:
                if probsat_output.clause_total != inst_clause_count:
                    raise ValueError(f"Clause count mismatch in probSAT output for file {cnf_filepath}: expected {inst_clause_count}, got {probsat_output.clause_total}")
                row_probsat = {
                    "ALG_algorithm": "probSAT",
                    "ALG_max_flips": max_flips,
                    "ALG_max_tries": max_tries,
                    "ALG_seed": random_seed,
                    "INST_n_variables": inst_var_count,
                    "INST_instance_id": inst_id,
                    "INST_clause_total": inst_clause_count,
                    "OUT_clause_satisfied": probsat_output.clause_satisfied,
                    "OUT_flips_count": probsat_output.iteration_count,
                }
            else:
                # TIMEOUT
                row_probsat = {
                    "ALG_algorithm": "probSAT",
                    "ALG_max_flips": max_flips,
                    "ALG_max_tries": max_tries,
                    "ALG_seed": random_seed,
                    "INST_n_variables": inst_var_count,
                    "INST_instance_id": inst_id,
                    "INST_clause_total": inst_clause_count,
                    "OUT_clause_satisfied": None,
                    "OUT_flips_count": None,
                }
            pd.DataFrame([row_probsat]).to_csv(result_filename, mode="a", header=not os.path.exists(result_filename), index=False)


if __name__ == "__main__":
    # run experiment with parameters

    # read arguments from command line

    parser = argparse.ArgumentParser(description="Run SAT solver experiments.")
    parser.add_argument("-r","--repeat_for_instances", type=int, default=5, help="Number of repetitions per instance.")
    parser.add_argument("-d","--data_num_instances_per_n", type=int, default=10, help="Number of instances per variable count n.")
    parser.add_argument("-t","--alg_max_tries", type=int, default=100, help="Maximum number of tries for the algorithms.")
    parser.add_argument("-ts","--alg_timeout_seconds", type=int, default=300, help="Timeout in seconds for each algorithm run.")
    args = parser.parse_args()
    run_experiment(
        repeat_for_instances=args.repeat_for_instances,
        data_num_instances_per_n=args.data_num_instances_per_n,
        alg_max_tries=args.alg_max_tries,
        alg_timeout_seconds=args.alg_timeout_seconds
    )