import os
import csv
import pandas as pd
import re
import datetime
import time
import subprocess
import multiprocessing as mp
from numpy import random
from time import sleep
import argparse
from pathlib import Path
#from scipy.stats import gmean

# Assuming metric_parser.py exists in the same directory or is importable
from metric_parser import parse_metrics #

parser = argparse.ArgumentParser()
parser.add_argument('--trace_dir', help='path to trace directory', required= True)
parser.add_argument('--results_dir', help='path to results directory', required= True)
parser.add_argument('--binary', default='./cbp', help='path to binary to run')

args = parser.parse_args()
trace_dir = Path(args.trace_dir)
results_dir = Path(args.results_dir)
binary = args.binary

def get_trace_paths(start_path):
    ret_list = []
    for root, dirs, files in os.walk(start_path):
        for my_file in files:
            if(my_file.endswith('_trace.gz')):
                ret_list.append(os.path.join(root, my_file))
    return ret_list

# Define the list of metric keys here, making it easily extensible
BASE_METRIC_KEYS = [
    'ExecTime',
    '_Instr', 
    '_Cycles',
    '_IPC',
    '_NumBr',
    '_MispBr', 
    '_BrPerCyc',
    '_MispBrPerCyc',
    '_MR',
    '_MPKI',
    '_CycWP',
    '_CycWPAvg',
    '_CycWPPKI',
    '_50PercInstr',
    '_50PercCycles',
    '_50PercIPC',
    '_50PercNumBr',
    '_50PercMispBr',
    '_50PercBrPerCyc',
    '_50PercMispBrPerCyc',
    '_50PercMR',
    '_50PercMPKI',
    '_50PercCycWP',
    '_50PercCycWPAvg',
    '_50PercCycWPPKI'
]

CUSTOM_METRICS = [
    'CSC preds',
    'RUNLTS preds'
]

METRIC_KEYS = BASE_METRIC_KEYS + CUSTOM_METRICS

def create_run_results_dict(wl_name, run_name, trace_size, pass_status_str, metrics_data, metric_keys):
    """
    Creates the dictionary of run results based on a list of metric keys.
    """
    retval = {
        'Workload': wl_name,
        'Run': run_name,
        'TraceSize': trace_size,
        'Status': pass_status_str,
    }

    for key in metric_keys:
        # Remove the leading underscore if present for display in the DataFrame
        # e.g., '_Instr' becomes 'Instr', '_50PercIPC' becomes '50PercIPC'
        display_key = key.lstrip('_') 
        retval[display_key] = metrics_data.get(key)
    
    return retval

def process_run_op(pass_status, my_trace_path, my_run_name, op_file):
    run_name_split = re.split(r"\/", my_run_name)
    wl_name = run_name_split[0]
    run_name = run_name_split[1]
    print(f'Extracting data from : {op_file} |  WL:{wl_name} | Run:{run_name}')
    
    trace_size = os.path.getsize(my_trace_path)/(1024 * 1024)

    pass_status_str = 'Fail'
    metrics = {} # Initialize metrics dictionary

    if pass_status:
        pass_status_str = 'Pass'
        metrics = parse_metrics(op_file) # Call the parsing function from metric_parser.py

    # Use the new function to create the return dictionary
    retval = create_run_results_dict(wl_name, run_name, trace_size, pass_status_str, metrics, METRIC_KEYS)
    
    return retval

my_traces = get_trace_paths(trace_dir)

print(f'Got {len(my_traces)} traces')

timestamp = datetime.datetime.now().strftime("%m_%d_%H-%M-%S")
if not os.path.exists(f'{results_dir}'):
    os.mkdir(results_dir)

def execute_trace(my_trace_path):
    assert(os.path.exists(my_trace_path))
        
    run_split = re.split(r"\/", my_trace_path)
    my_wl = run_split[-2] 
    # traces/int/int_0_trace.gz
    run_name = run_split[-1].split(".")[-2]
    if not os.path.exists(f'{results_dir}/{my_wl}'):
        if not os.path.exists(f'{results_dir}/{my_wl}'):
            os.makedirs(f'{results_dir}/{my_wl}', exist_ok=True)

    do_process = True
    my_run_name = f'{my_wl}/{run_name}'
    exec_cmd = f'{binary} {my_trace_path}'
    op_file = f'{results_dir}/{my_wl}/{run_name}.txt'
    if os.path.exists(op_file):
        #print(f"OP file:{op_file} already exists. Not running again!")
        do_process = False
    #
    pass_status = True
    if do_process:
        print(f'Begin processing run:{my_run_name}')
        try:
            begin_time = time.time()
            run_op = subprocess.check_output(exec_cmd, shell=True, text=True)
            end_time = time.time()
            exec_time = end_time - begin_time
            with open(op_file, "w") as text_file:
                print(f"CMD:{exec_cmd}", file=text_file)
                print(f"{run_op}", file=text_file)
                print(f"ExecTime = {exec_time}", file=text_file)
        except:
            print(f'Run: {my_run_name} failed')
            pass_status = False
    return(pass_status, my_trace_path, op_file, my_run_name)



if __name__ == '__main__':
    # For parallel runs:
    with mp.Pool() as pool:
        results = pool.map(execute_trace, my_traces)
    
    # For serial runs:
    #results = []
    #for my_trace in my_traces:
    #    results.append(execute_trace(my_trace))
    
    # Dynamically build DataFrame columns based on what's returned by process_run_op
    # Get all possible keys from the first successful run_dict to define columns
    initial_run_dict = {}
    for my_result in results:
        pass_status = my_result[0]
        trace_path = my_result[1]
        op_file = my_result[2]
        my_run_name = my_result[3]
        if pass_status: # Only get keys from a successful run to ensure all metrics are present
            # Call process_run_op here to get the full dictionary structure
            initial_run_dict = process_run_op(pass_status, trace_path, my_run_name, op_file)
            break # Found a successful run, break to use its keys

    if initial_run_dict:
        df_columns = list(initial_run_dict.keys())
    else:
        # Fallback to a predefined list if no successful run to get keys from
        # Ensure this fallback matches the keys generated by create_run_results_dict
        df_columns = ['Workload', 'Run', 'TraceSize', 'Status'] + [key.lstrip('_') for key in METRIC_KEYS]
    
    df = pd.DataFrame(columns=df_columns) #

    for my_result in results:
        pass_status = my_result[0]
        trace_path = my_result[1]
        op_file = my_result[2]
        my_run_name = my_result[3]
        run_dict = process_run_op(pass_status, trace_path, my_run_name, op_file) #
        my_df = pd.DataFrame([run_dict])
        if not df.empty:
            df = pd.concat([df, my_df], ignore_index=True)
        else:
            df = my_df.copy()
    print(df) #
    df.to_csv(f'{results_dir}/results.csv', index=False) #
    
    
    unique_wls = df['Workload'].unique() #
    
    print('\n\n----------------------------------Aggregate Metrics Per Workload Category----------------------------------\n') #
    for my_wl in unique_wls:
        # Check if column exists AND if it contains non-None values before converting to float and calculating mean
        if '50PercMPKI' in df.columns and df[df['Workload'] == my_wl]['50PercMPKI'].dropna().empty == False:
            my_wl_br_misp_pki_amean = df[df['Workload'] == my_wl]['50PercMPKI'].astype(float).mean() #
            print(f'WL:{my_wl:<10} Branch Misprediction PKI(BrMisPKI) AMean : {my_wl_br_misp_pki_amean}') #
        else:
            print(f'WL:{my_wl:<10} Branch Misprediction PKI(BrMisPKI) AMean : N/A (Missing or all None values)')
            
        if '50PercCycWPPKI' in df.columns and df[df['Workload'] == my_wl]['50PercCycWPPKI'].dropna().empty == False:
            my_wl_cyc_wp_pki_amean = df[df['Workload'] == my_wl]['50PercCycWPPKI'].astype(float).mean() #
            print(f'WL:{my_wl:<10} Cycles On Wrong-Path PKI(CycWpPKI) AMean : {my_wl_cyc_wp_pki_amean}') #
        else:
            print(f'WL:{my_wl:<10} Cycles On Wrong-Path PKI(CycWpPKI) AMean : N/A (Missing or all None values)')
    print('-----------------------------------------------------------------------------------------------------------') #
    
    br_misp_pki_amean = 'N/A'
    cyc_wp_pki_amean = 'N/A'

    if '50PercMPKI' in df.columns and df['50PercMPKI'].dropna().empty == False:
        br_misp_pki_amean = df['50PercMPKI'].astype(float).mean() #
    if '50PercCycWPPKI' in df.columns and df['50PercCycWPPKI'].dropna().empty == False:
        cyc_wp_pki_amean = df['50PercCycWPPKI'].astype(float).mean() #
    #ipc_geomean = df['50PercIPC'].astype(float).apply(gmean)
    
    print('\n\n---------------------------------------------Aggregate Metrics---------------------------------------------\n') #
    print(f'Branch Misprediction PKI(BrMisPKI) AMean : {br_misp_pki_amean}') #
    print(f'Cycles On Wrong-Path PKI(CycWpPKI) AMean : {cyc_wp_pki_amean}') #
    print('-----------------------------------------------------------------------------------------------------------') #

    print('\n\n---------------------------------------------Aggregate Metrics (CUSTOM)---------------------------------------------\n') #
    # Aggregate Custom Metrics Overall
    for custom_metric_key in CUSTOM_METRICS:
        display_key = custom_metric_key.lstrip('_')
        custom_metric_overall_agg = 'N/A'
        if display_key in df.columns and not df[display_key].dropna().empty:
            # Check if the values contain '%'
            # Convert to string first to use .str accessor, then fillna('') to handle NaNs during string operations
            temp_series = df[display_key].astype(str).str.contains('%', na=False)
            
            if temp_series.any(): # If any value contains '%'
                # Remove '%' and convert to float, then calculate mean
                custom_metric_overall_agg = df[display_key].astype(str).str.replace('%', '', regex=False).astype(float).mean()
                print(f'{display_key} AMean (Percentage) : {custom_metric_overall_agg}')
            else:
                # Otherwise, calculate sum
                custom_metric_overall_agg = df[display_key].astype(float).sum()
                print(f'{display_key} Sum : {custom_metric_overall_agg}')
        else:
            print(f'{display_key} : N/A')
    print('--------------------------------------------------------------------------------------------------------------------') #
