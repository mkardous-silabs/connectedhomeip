#!/bin/sh

#Filenames to exclude from the check
exclude=( 
    index.md 
    DOC_STANDARD.md 
    README.md
    )

#File paths for all .md files in the Silabs doc
md_paths=( ../*.md ../**/*.md )

#All docleafs
md_docleafs=( ./_docleaf*.yml )

#Do the check
for md_path in "${md_paths[@]}";
do
   #Get the filename for the .md file
   md_file=$(basename $md_path)
   #If the filename is not excluded
   if [[ ! " ${exclude[*]} " =~ " $md_file " ]]; then
        #search foe the file in the docleafs
        found=""
        for md_docleaf in "${md_docleafs[@]}";
        do
            found=$(grep $md_file $md_docleaf)
            if [[ $found != "" ]]; then 
                break
            fi
        done
        if [[ $found == "" ]]; then
            echo "File check FAILED! $md_path not found in any docleaf"
            echo " either add it to the exceptions in ./docs/silabs/.suds/check.sh"
            echo " or add it into a docleaf so that it will show up in the left"
            echo " hand navigation on DSC."
            exit 1
        fi
   fi
done
