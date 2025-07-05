# metric_parser.py
import re

def extract_metric_value(metric_name, line):
    """
    Searches for 'metric_name: val' in the line and returns 'val'.
    'val' is everything after ': ' to the end of the line (stripped).
    Returns None if the metric name is not found in the line or the format doesn't match.
    """
    # First, a quick check to see if the metric_name string is present in the line
    if metric_name not in line:
        return None

    pattern = rf'{re.escape(metric_name)}: (.*)'
    match = re.search(pattern, line)
    if match:
        return match.group(1).strip()
    return None

def add_metric_if_found(metrics_dict, metric_key, line_to_search):
    """
    Attempts to extract a metric value from a line using extract_metric_value.
    If a value is found, it updates the metrics_dict with the metric_key and its value.
    """
    value = extract_metric_value(metric_key, line_to_search)
    if value is not None:
        metrics_dict[metric_key] = value

def parse_metrics(op_file):
    """
    Parses the given output file and extracts relevant metrics.
    Returns a dictionary of metrics.
    """
    metrics = {}
    
    _50perc_section_header = 'DIRECT CONDITIONAL BRANCH PREDICTION MEASUREMENTS (50 Perc instructions)'
    _100perc_section_header = 'DIRECT CONDITIONAL BRANCH PREDICTION MEASUREMENTS (Full Simulation i.e. Counts Not Reset When Warmup Ends)'

    process_50perc_section = False
    process_100perc_section = False
    found_50perc_line_to_process = False
    found_100perc_line_to_process = False

    with open(op_file, "r") as text_file:
        for line in text_file:
            if not line.strip():
                continue

            # Using the new encapsulated function for ExecTime
            add_metric_if_found(metrics, "ExecTime", line)
            add_metric_if_found(metrics, "CSC preds", line)
            add_metric_if_found(metrics, "RUNLTS preds", line)
            add_metric_if_found(metrics, "CSC coverage", line)
            add_metric_if_found(metrics, "RUNLTS coverage", line)
            add_metric_if_found(metrics, "CSC accuracy", line)
            add_metric_if_found(metrics, "RUNLTS accuracy", line)

            if(not process_50perc_section and _50perc_section_header in line):
                process_50perc_section = True
                process_100perc_section = False

            if(not process_100perc_section and _100perc_section_header in line):
                process_50perc_section = False
                process_100perc_section = True

            if process_50perc_section:
                if (found_50perc_line_to_process):
                    curr_split_line = line.split()
                    metrics['_50PercInstr']        = curr_split_line[0]
                    metrics['_50PercCycles']       = curr_split_line[1]
                    metrics['_50PercIPC']          = curr_split_line[2]
                    metrics['_50PercNumBr']        = curr_split_line[3]
                    metrics['_50PercMispBr']       = curr_split_line[4]
                    metrics['_50PercBrPerCyc']     = curr_split_line[5]
                    metrics['_50PercMispBrPerCyc']= curr_split_line[6]
                    metrics['_50PercMR']           = curr_split_line[7]
                    metrics['_50PercMPKI']         = curr_split_line[8]
                    metrics['_50PercCycWP']        = curr_split_line[9]
                    metrics['_50PercCycWPAvg']     = curr_split_line[10]
                    metrics['_50PercCycWPPKI']     = curr_split_line[11]
                    process_50perc_section = False
                    found_50perc_line_to_process = False

                if(all(x in line for x in ['Instr', 'Cycles', 'IPC', 'NumBr', 'MispBr', 'BrPerCyc', 'MispBrPerCyc', 'MR', 'MPKI', 'CycWP', 'CycWPAvg'])):
                    found_50perc_line_to_process = True

            if process_100perc_section:
                if (found_100perc_line_to_process):
                    curr_split_line = line.split()
                    metrics['_Instr']           = curr_split_line[0]
                    metrics['_Cycles']          = curr_split_line[1]
                    metrics['_IPC']             = curr_split_line[2]
                    metrics['_NumBr']           = curr_split_line[3]
                    metrics['_MispBr']          = curr_split_line[4]
                    metrics['_BrPerCyc']        = curr_split_line[5]
                    metrics['_MispBrPerCyc']    = curr_split_line[6]
                    metrics['_MR']              = curr_split_line[7]
                    metrics['_MPKI']            = curr_split_line[8]
                    metrics['_CycWP']           = curr_split_line[9]
                    metrics['_CycWPAvg']        = curr_split_line[10]
                    metrics['_CycWPPKI']        = curr_split_line[11]
                    process_100perc_section = False
                    found_100perc_line_to_process = False
                if(all(x in line for x in ['Instr', 'Cycles', 'IPC', 'NumBr', 'MispBr', 'BrPerCyc', 'MispBrPerCyc', 'MR', 'MPKI', 'CycWP', 'CycWPAvg'])):
                    found_100perc_line_to_process = True
    return metrics
