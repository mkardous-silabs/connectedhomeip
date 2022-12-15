./check.sh
if [ $? -eq 1 ]; then 
    exit 1
fi
./compile.sh
suds generate-pdf -c _matter_smg_docspace.yml -o ./out