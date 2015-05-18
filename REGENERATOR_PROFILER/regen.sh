#	Code Regenerator
#	Author: Seongsu Lee(su880214@hanyang.ac.kr)
#	Embedded Software System Laboratory, Hanyang University
#!/bin/bash

argc=$#
argv0=$0
argv1=$1 #option


# check profiling result of the source code that a user wants to regenerate

####	profile result file	####
profile_file="heapo_gen_result.csv"
profile_result=$(ls | grep $profile_file)

if [ -z "$profile_result" ]; then

	# If no profile result, the renerator is terminated with returning value -1
        echo ERROR: No profile result
        exit -1
fi


# get current directory name 
dir=($(pwd | awk '{split($0, info, "/"); for(i = 2; info[i] != null; i++){  } printf("\n%s\n",info[i-1]);}'))	

#### 	object name		####
object=("\""$dir"_obj\"")

#### 	pos_create		####
pcreate="{\n\tif(pos_create($object) < 1){ \n\t\tif(pos_map($object) < 1){\n\t\t\treturn 0;\n\t\t} \n\t}"

#### 	pos-lib.h		####
pheader="#include \"pos-lib.h\""

#### 	pos_malloc  		####
pmalloc="pos_malloc($object, "

#### 	POS AREA region		####
pos_start="0x5FFEF8000000"
pos_end="0x7FFEF8000000"


#### 	file extension		####
extension=".c"

#### 	sed variables		####
s="s"
entry=1
index="1"
add="0"
p="p"

# It makes a source file list from current directory and sub directories
find "`pwd`" -name "*"$extension -print > files.regen

# It takes a first file from the list
file=$(sed -n '1p' files.regen)

#### 	backup files 		####
backup=($dir"_origin")
mkdir /tmp/regen
cp -R * /tmp/regen/
mkdir $backup
mv /tmp/regen/* $backup
rm -r /tmp/regen
rm -r $backup


####	first entry for malloc	####
index="1"
mdata=$(sed -n $index$p $profile_file)


####	info[0]=file name	#### 
####	info[1]=line number	#### 
####	info[2]=func name	####
info=($(echo $mdata | awk '{ split($0,info,":"); printf("%s %s %s\n", info[1],info[2],info[3]);}'))	
prvfile=$file
#echo here
####	tour all entries for malloc	####
while [ -n "$mdata" ]; do

	line=(${info[1]})
	file=(${info[0]})
	prvfile=($file) 

	####	malloc substitution ####	
	sed -i "$line$s/malloc[^A-Za-z0-9_$]*(/$pmalloc/" $file

	if [ "$prvfile" != "$file" ] ; then
		firstline=$(sed -n 1p $prvfile)		

		if [ "$firstline" != "$pheader" ]; then

			echo $pheader | cat > temp
			cat $prvfile >> temp
			rm $prvfile
			mv temp $prvfile
							
		fi
	fi

	####	next entry	####
	let index++ 
	mdata=$(sed -n $index$p $profile_file)	
	info=($(echo $mdata | awk '{ split($0,info,":"); printf("%s %s %s\n", info[1],info[2],info[3]);}'))	

done


firstline=$(sed -n 1p $prvfile)		

if [ "$firstline" != "$pheader" ]; then

	echo $pheader | cat > temp
	cat $prvfile >> temp
	rm $prvfile
	mv temp $prvfile

fi
	
mainmodif=0	
freemodif=0		



####	tour all files		####
while [ -n "$file" ]; do

	
	####	lookup main		####
	main1=$(sed -n '/\smain\s*(/p' $file)
	#case2=$(sed -n '/^mains*(/p' $file)
	
	if [ -n "$main1" ]; then

		# get the line number where main func starts
		curly=$(echo $main1 | sed -n -e '/{/p')
		line=$(sed -n '/\smain\s*(/=' $file)

		# look up each line for curly bracket
		while [ -z "$curly" ]; do
			
			line=`expr "$line" "+" "1"`	
			curly=$(sed -n $line'p' $file | sed -n -e '/{/p')
		done

		# insert pos_create at the start line of main func	
		sed -i "$line$s/{/$pcreate/" $file
		modif=1
		mainmodif=1

	fi

	frees1=$(sed -n '/^\(\s.\)*.*[^A-Za-z0-9_$\*\/]free[\_s]*(.*);/=' $file)
	
	if [ -n "$frees1" ]; then 
		free=($(echo $frees1 | awk '{split($0, info, " "); for(i = 1; info[i] !=null; i++){ printf("%s ",info[i]);}}'))
			
		index="0"

		while [ -n "${free[$index]}" ]; do	
	
			add=`expr "$index" "*" 4`
			free[$index]=`expr "${free[$index]}" "+" "$add"`
			let index++
		done
		
		index="0"
	
		while [ -n "${free[$index]}" ]; do


        		#echo get sing free line
        		line=$(sed -n "${free[$index]}p" $file)
				
        		####    parenthesis extraction  ####
        		index1=$(echo `expr index "$line" "\("`)
        		index2=$(echo `expr index "$line" "\)"`)

			####	rest string after free	####
			rest=$(echo ${line:$index2})

        		####    variable lentgh 	####
        		varlen=`expr "$index2" "-" "$index1" "-" "1"`

        		####    variable extraction     ####
        		variable=$(echo ${line:$index1:$varlen})
				
			#echo [$index]${free[$index]} : $line : $variable

	
        		####    pos_free  string        ####
        		pfree="\tif($variable < $pos_end \&\& $variable >= $pos_start){\n\t\tpos_free($object,$variable)$rest\n\t}else{\n\t\tfree($variable)$rest\n\t}"


			####	free substitution 	####
				#echo free_substituion
        			sed -i "${free[$index]}$s/free.*;/$pfree/" $file
	
        		let index++

		done
		freemodif=1
		modif=1
	fi



	####	insert pos header 	####
	if [  $modif ]; then
			
		#echo get first line for pos header
		firstline=$(sed -n 1p $file)
		
		if [ "$firstline" != "$pheader" ]; then

			echo $pheader | cat > temp
			cat $file >> temp
			rm $file
			mv temp $file	
							
		fi
		
		echo modified: $file
		if [ $mainmodif ]; then
			echo 	insertion: pos_create
			mainmodif=0
		fi

		if [ $freemodif ]; then
			echo 	regeneration: pos_free
			freemodif=0		
		fi
	
		modif=0
	fi


	####	move to next file	####
	let entry++
	file=$(sed -n $entry'p' files.regen)

done

#rm files.regen
