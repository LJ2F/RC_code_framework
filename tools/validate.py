"""Build the firmware matrix and run native tests. Logs stay under build/validation."""
from pathlib import Path
import json,shutil,subprocess,sys,re
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"build/validation"
def run(args,log):
    result=subprocess.run([str(a) for a in args],cwd=ROOT,text=True,
                          encoding="utf-8",errors="replace",stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    with log.open("a",encoding="utf-8") as f:
        f.write("$ "+" ".join(map(str,args))+"\n"+result.stdout+"\n")
    if result.returncode:
        print(result.stdout[-16000:]);raise RuntimeError(f"Command failed; see {log}")
    return result.stdout
def main():
    OUT.mkdir(parents=True,exist_ok=True)
    for tool in ("cmake","ninja","arm-none-eabi-gcc","arm-none-eabi-g++","gcc","g++"):
        if not shutil.which(tool):raise RuntimeError(f"Missing PATH tool: {tool}")
    results={"toolchains":{},"firmware":[],"host":[]}
    for tool in ("arm-none-eabi-gcc","g++"):
        results["toolchains"][tool]=subprocess.check_output([tool,"--version"],text=True).splitlines()[0]
    for config in ("Debug","Release"):
        for usb in ("ON","OFF"):
            name=f"{config}-usb-{usb.lower()}";dest=OUT/name;log=OUT/(name+".log")
            log.write_text("",encoding="utf-8");print(f"Building {name}",flush=True)
            run(["cmake","--fresh","-S",ROOT,"-B",dest,"-G","Ninja",
                 "-DCMAKE_TOOLCHAIN_FILE="+str(ROOT/"cmake/gcc-arm-none-eabi.cmake"),
                 "-DCMAKE_BUILD_TYPE="+config,"-DRC_FRAMEWORK_ENABLE_USB="+usb],log)
            output=run(["cmake","--build",dest,"--clean-first","--parallel","8","--target",
                        "RC_code_framework","framework_header_check","framework_example_check"],log)
            commands=json.loads((dest/"compile_commands.json").read_text(encoding="utf-8"))
            for entry in commands:
                if entry["file"].endswith(".cpp"):
                    assert "-std=c++23" in entry["command"],entry["file"]
                elif entry["file"].endswith(".c"):
                    assert "-std=c11" in entry["command"],entry["file"]
            if usb=="OFF":
                assert not any("USB_DEVICE" in c["file"] or "protocol_usb_example" in c["file"] or
                               "/BSP/UsbPort.cpp" in c["file"].replace("\\","/") for c in commands)
            memory=dict((k,int(v)) for k,v in re.findall(r"(\w+):\s+(\d+) B",output))
            results["firmware"].append({"configuration":name,"passed":True,"memory_bytes":memory,
                                       "translation_units":len(commands)})
            print(f"  passed: {memory}",flush=True)
        name="host-"+config.lower();dest=OUT/name;log=OUT/(name+".log")
        log.write_text("",encoding="utf-8")
        run(["cmake","--fresh","-S",ROOT/"tests","-B",dest,"-G","Ninja",
             "-DCMAKE_C_COMPILER="+shutil.which("gcc"),"-DCMAKE_CXX_COMPILER="+shutil.which("g++"),
             "-DCMAKE_BUILD_TYPE="+config],log)
        run(["cmake","--build",dest,"--clean-first","--parallel","8"],log)
        output=run(["ctest","--test-dir",dest,"--output-on-failure","-V"],log)
        results["host"].append({"configuration":name,"passed":True})
        print(f"  {name}: all tests passed",flush=True)
    (OUT/"results.json").write_text(json.dumps(results,indent=2),encoding="utf-8")
    print("All checks passed. Results: "+str(OUT/"results.json"),flush=True)
if __name__=="__main__":
    try:main()
    except Exception as exc:
        print(str(exc),file=sys.stderr);sys.exit(1)
