test_dir="test/"
tests=("test00" "test01" "test02" "test03" "test04" "test05" "test06" "test07" 
"test08" "test09" "test10" "test11" "test12" "test13" "test14" "test15" 
"test16" "test17" "test18" "test19" "test20" "test21" "test22" "test23" 
"test24" "test25")
results=(100 10 20 200 10 10 20 10 20 20 5 100 1 1 1 1 1 1 1 1 1 1 1 1 1)

for i in {0..24};
do
    test=${tests[$i]}
    expected=${results[$i]}
    test_path="${test_dir}${test}.c"
    
    ./build/ast-interpreter "$(cat $test_path)" 2>tmp.txt
    res=$(cat tmp.txt)
    # echo $res
    # echo $expected
    if [ "$expected" = "$res" ];then
        echo "* success $test"
    else 
        echo "x failed $test"
        break
    fi
done
rm tmp.txt
