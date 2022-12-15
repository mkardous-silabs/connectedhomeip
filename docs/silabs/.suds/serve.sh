./check.sh
if [ $? -eq 1 ]; then 
    exit 1
fi
./compile.sh
suds serve -c _docspace-sld56-matter.yml -o ./out