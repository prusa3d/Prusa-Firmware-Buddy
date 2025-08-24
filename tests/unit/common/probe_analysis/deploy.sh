set -e

function run_in_jupyter() {
    echo jupyter: $@
    ssh prusa-nibbler "docker exec jupyter_jupyter_1 $@"
}

scp python/probe_analysis.cpp python/setup.py prusa-nibbler:services/jupyter/share
scp ../../../../src/common/probe_analysis.hpp prusa-nibbler:services/jupyter/share
run_in_jupyter cp /share/probe_analysis.hpp probe_analysis/probe_analysis.hpp
run_in_jupyter cp /share/probe_analysis.cpp probe_analysis/probe_analysis.cpp
run_in_jupyter cp /share/setup.py probe_analysis/setup.py
run_in_jupyter pip install -e ./probe_analysis
