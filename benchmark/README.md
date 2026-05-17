## string

| Benchmark | Time (ns) | CPU (ns) | Iterations |
|-----------|-----------|----------|-------------|
| BM_Construct_Short_KString | 5.49 | 5.49 | 119169429 |
| BM_Construct_Short_StdString | 16.0 | 16.0 | 42724479 |
| BM_Construct_Medium_KString | 6.82 | 6.82 | 100772541 |
| BM_Construct_Medium_StdString | 17.0 | 17.0 | 40168290 |
| BM_Construct_Long_KString | 15.3 | 15.3 | 44762435 |
| BM_Construct_Long_StdString | 44.1 | 44.1 | 15977197 |
| BM_Copy_Short_KString | 6.57 | 6.57 | 102938234 |
| BM_Copy_Short_StdString | 15.7 | 15.7 | 43955526 |
| BM_Copy_Medium_KString | 13.2 | 13.2 | 51829189 |
| BM_Copy_Medium_StdString | 16.3 | 16.3 | 40902013 |
| BM_Copy_Long_KString | 27.0 | 27.0 | 26023906 |
| BM_Copy_Long_StdString | 34.8 | 34.8 | 20561268 |
| BM_Move_Short_KString | 6.42 | 6.43 | 105445061 |
| BM_Move_Short_StdString | 16.9 | 16.9 | 42416506 |
| BM_Move_Medium_KString | 8.47 | 8.47 | 76874902 |
| BM_Move_Medium_StdString | 19.7 | 19.7 | 37754463 |
| BM_Move_Long_KString | 19.1 | 19.1 | 37339600 |
| BM_Move_Long_StdString | 49.4 | 49.5 | 14449232 |
| BM_Assign_Short_KString | 1.77 | 1.77 | 373727220 |
| BM_Assign_Short_StdString | 4.44 | 4.44 | 147831558 |
| BM_Append_Short_KString | 2.47 | 2.47 | 284421248 |
| BM_Append_Short_StdString | 3.82 | 3.82 | 181684890 |
| BM_Append_Medium_KString | 3.79 | 3.79 | 184012675 |
| BM_Append_Medium_StdString | 4.48 | 4.48 | 132442342 |
| BM_Append_Long_KString | 13.5 | 13.5 | 50576323 |
| BM_Append_Long_StdString | 13.5 | 13.5 | 51504077 |
| BM_Find_Short_KString | 11.0 | 11.0 | 64307746 |
| BM_Find_Short_StdString | 7.98 | 7.98 | 84419900 |
| BM_Find_Long_KString | 93.3 | 93.3 | 7628344 |
| BM_Find_Long_StdString | 106 | 106 | 6296770 |
| BM_Compare_Equal_Short_KString | 1.42 | 1.42 | 488153471 |
| BM_Compare_Equal_Short_StdString | 1.59 | 1.59 | 446753739 |
| BM_Compare_NotEqual_Medium_KString | 1.30 | 1.30 | 547173924 |
| BM_Compare_NotEqual_Medium_StdString | 1.27 | 1.27 | 555764219 |
| BM_Hash_Short_KString | 3.30 | 3.31 | 215456636 |
| BM_Hash_Short_StdString | 3.27 | 3.27 | 216539317 |
| BM_Hash_Long_KString | 227 | 227 | 2783845 |
| BM_Hash_Long_StdString | 231 | 231 | 3017936 |

## vector map

以下是将您提供的基准测试结果整理成的 Markdown 表格：

| Benchmark | Time (ns) | CPU (ns) | Iterations | UserCounters |
|-----------|-----------|----------|------------|---------------|
| VectorMap/string_batch_build/100/100 | 5302 | 5301 | 128797 | items_per_second=18.8627M/s |
| VectorMap/string_insert_ordered/100/100 | 5040 | 5040 | 123486 | items_per_second=19.8411M/s |
| StdMap/string_insert_ordered/100/100 | 6739 | 6739 | 96985 | items_per_second=14.8381M/s |
| VectorSet/string_batch_build/100/100 | 4731 | 4732 | 151899 | items_per_second=21.1347M/s |
| VectorMap/insert_ordered/1000 | 9201 | 9202 | 73902 | items_per_second=108.677M/s |
| VectorMap/insert_ordered/10000 | 124928 | 124935 | 5406 | items_per_second=80.0416M/s |
| VectorMap/insert_ordered/100000 | 1539220 | 1539308 | 442 | items_per_second=64.9643M/s |
| VectorMap/insert_ordered/500000 | 8963869 | 8964380 | 76 | items_per_second=55.7763M/s |
| VectorMap/insert_random/1000 | 83852 | 83857 | 7925 | items_per_second=11.925M/s |
| VectorMap/insert_random/10000 | 5001411 | 5001654 | 136 | items_per_second=1.99934M/s |
| VectorMap/insert_random/100000 | 434847141 | 434871195 | 2 | items_per_second=229.953k/s |
| VectorMap/insert_random/500000 | 1.5031e+10 | 1.5031e+10 | 1 | items_per_second=33.2641k/s |
| VectorMap/find_existing/1000 | 24864 | 24866 | 25825 | items_per_second=40.2162M/s |
| VectorMap/find_existing/10000 | 610472 | 610390 | 1090 | items_per_second=16.383M/s |
| VectorMap/find_existing/100000 | 8440845 | 8441216 | 81 | items_per_second=11.8466M/s |
| VectorMap/find_existing/500000 | 61632126 | 61634698 | 11 | items_per_second=8.11231M/s |
| VectorMap/find_missing/1000 | 5398 | 5399 | 125850 | items_per_second=185.229M/s |
| VectorMap/find_missing/10000 | 78809 | 78813 | 8522 | items_per_second=126.883M/s |
| VectorMap/find_missing/100000 | 990239 | 990194 | 717 | items_per_second=100.99M/s |
| VectorMap/find_missing/500000 | 5580246 | 5580002 | 119 | items_per_second=89.6057M/s |
| VectorMap/iterate/1000 | 151 | 151 | 4647473 | items_per_second=6.61899G/s |
| VectorMap/iterate/10000 | 1405 | 1399 | 511331 | items_per_second=7.14689G/s |
| VectorMap/iterate/100000 | 13769 | 13769 | 49250 | items_per_second=7.26251G/s |
| VectorMap/iterate/500000 | 72086 | 72091 | 9417 | items_per_second=6.93572G/s |
| VectorMap/erase_by_key/1000 | 179399 | 179413 | 3788 | items_per_second=5.57375M/s |
| VectorMap/erase_by_key/10000 | 9676022 | 9676430 | 70 | items_per_second=1.03344M/s |
| VectorMap/erase_by_key/100000 | 878727451 | 878753415 | 1 | items_per_second=113.798k/s |
| VectorMap/erase_by_key/500000 | 2.9119e+10 | 2.9119e+10 | 1 | items_per_second=17.171k/s |
| StdMap/insert_ordered/1000 | 35279 | 35277 | 20004 | items_per_second=28.3472M/s |
| StdMap/insert_ordered/10000 | 446145 | 446170 | 1573 | items_per_second=22.413M/s |
| StdMap/insert_ordered/100000 | 9409905 | 9410284 | 78 | items_per_second=10.6267M/s |
| StdMap/insert_ordered/500000 | 68248435 | 68251431 | 9 | items_per_second=7.32585M/s |
| StdMap/insert_random/1000 | 26312 | 26313 | 26600 | items_per_second=38.0046M/s |
| StdMap/insert_random/10000 | 892758 | 892812 | 761 | items_per_second=11.2006M/s |
| StdMap/insert_random/100000 | 16198161 | 16154106 | 43 | items_per_second=6.19038M/s |
| StdMap/insert_random/500000 | 213856642 | 213856136 | 5 | items_per_second=2.33802M/s |
| StdMap/find_existing/1000 | 22450 | 22450 | 30681 | items_per_second=44.5436M/s |
| StdMap/find_existing/10000 | 748708 | 748753 | 922 | items_per_second=13.3555M/s |
| StdMap/find_existing/100000 | 15873875 | 15874613 | 46 | items_per_second=6.29937M/s |
| StdMap/find_existing/500000 | 178375959 | 178214943 | 4 | items_per_second=2.8056M/s |
| StdMap/find_missing/1000 | 8819 | 8819 | 79268 | items_per_second=113.386M/s |
| StdMap/find_missing/10000 | 150870 | 150879 | 4624 | items_per_second=66.2783M/s |
| StdMap/find_missing/100000 | 2054566 | 2054675 | 326 | items_per_second=48.6695M/s |
| StdMap/find_missing/500000 | 31197181 | 31199002 | 24 | items_per_second=16.0262M/s |
| StdMap/iterate/1000 | 3565 | 3565 | 204508 | items_per_second=280.473M/s |
| StdMap/iterate/10000 | 37856 | 37858 | 16233 | items_per_second=264.143M/s |
| StdMap/iterate/100000 | 416197 | 416218 | 1482 | items_per_second=240.259M/s |
| StdMap/iterate/500000 | 3076408 | 3076526 | 256 | items_per_second=162.521M/s |
| StdMap/erase_by_key/1000 | 134748 | 134777 | 5347 | items_per_second=7.41964M/s |
| StdMap/erase_by_key/10000 | 1845533 | 1845623 | 367 | items_per_second=5.41822M/s |
| StdMap/erase_by_key/100000 | 36338890 | 36339106 | 22 | items_per_second=2.75186M/s |
| StdMap/erase_by_key/500000 | 293229135 | 293247250 | 2 | items_per_second=1.70505M/s |
| VectorSet/insert_ordered/1000 | 9123 | 9123 | 77063 | items_per_second=109.612M/s |
| VectorSet/insert_ordered/10000 | 119879 | 119884 | 5596 | items_per_second=83.4137M/s |
| VectorSet/insert_ordered/100000 | 1525309 | 1525350 | 453 | items_per_second=65.5587M/s |
| VectorSet/insert_ordered/500000 | 8784545 | 8784999 | 78 | items_per_second=56.9152M/s |
| VectorSet/insert_random/1000 | 30834 | 30835 | 22635 | items_per_second=32.4307M/s |
| VectorSet/insert_random/10000 | 1256084 | 1256148 | 598 | items_per_second=7.96084M/s |
| VectorSet/insert_random/100000 | 142258544 | 142266765 | 5 | items_per_second=702.905k/s |
| VectorSet/insert_random/500000 | 4569548624 | 4569734870 | 1 | items_per_second=109.416k/s |
| VectorSet/find_existing/1000 | 29909 | 29911 | 22864 | items_per_second=33.4329M/s |
| VectorSet/find_existing/10000 | 567735 | 567764 | 1163 | items_per_second=17.613M/s |
| VectorSet/find_existing/100000 | 7781970 | 7782354 | 88 | items_per_second=12.8496M/s |
| VectorSet/find_existing/500000 | 50637536 | 50639611 | 13 | items_per_second=9.87369M/s |
| VectorSet/find_missing/1000 | 5743 | 5743 | 122201 | items_per_second=174.122M/s |
| VectorSet/find_missing/10000 | 81326 | 81325 | 8184 | items_per_second=122.963M/s |
| VectorSet/find_missing/100000 | 1020912 | 1020954 | 680 | items_per_second=97.9476M/s |
| VectorSet/find_missing/500000 | 5574797 | 5575118 | 117 | items_per_second=89.6842M/s |
| VectorSet/iterate/1000 | 149 | 149 | 4714377 | items_per_second=6.73329G/s |
| VectorSet/iterate/10000 | 1355 | 1355 | 510912 | items_per_second=7.37952G/s |
| VectorSet/iterate/100000 | 13474 | 13475 | 51987 | items_per_second=7.42142G/s |
| VectorSet/iterate/500000 | 67416 | 67418 | 10212 | items_per_second=7.41642G/s |
| VectorSet/erase_by_key/1000 | 95238 | 95245 | 7113 | items_per_second=10.4992M/s |
| VectorSet/erase_by_key/10000 | 23243408 | 23244254 | 30 | items_per_second=430.214k/s |
| VectorSet/erase_by_key/100000 | 2540067003 | 2540169900 | 1 | items_per_second=39.3674k/s |
| VectorSet/erase_by_key/500000 | 6.0639e+10 | 6.0639e+10 | 1 | items_per_second=8.24555k/s |
| StdSet/insert_ordered/1000 | 35314 | 35315 | 19809 | items_per_second=28.3167M/s |
| StdSet/insert_ordered/10000 | 419968 | 419983 | 1689 | items_per_second=23.8105M/s |
| StdSet/insert_ordered/100000 | 6659566 | 6659675 | 98 | items_per_second=15.0157M/s |
| StdSet/insert_ordered/500000 | 53290463 | 53292849 | 12 | items_per_second=9.38212M/s |
| StdSet/insert_random/1000 | 23957 | 23958 | 28987 | items_per_second=41.7397M/s |
| StdSet/insert_random/10000 | 834281 | 834299 | 802 | items_per_second=11.9861M/s |
| StdSet/insert_random/100000 | 14536729 | 14537008 | 48 | items_per_second=6.87899M/s |
| StdSet/insert_random/500000 | 146044150 | 146048223 | 6 | items_per_second=3.42353M/s |
| StdSet/find_existing/1000 | 29469 | 29470 | 23224 | items_per_second=33.9328M/s |
| StdSet/find_existing/10000 | 760094 | 760123 | 906 | items_per_second=13.1558M/s |
| StdSet/find_existing/100000 | 13735033 | 13734923 | 49 | items_per_second=7.28071M/s |
| StdSet/find_existing/500000 | 136907249 | 136908162 | 5 | items_per_second=3.65208M/s |
| StdSet/find_missing/1000 | 8077 | 8077 | 83528 | items_per_second=123.814M/s |
| StdSet/find_missing/10000 | 134264 | 134262 | 5012 | items_per_second=74.4814M/s |
| StdSet/find_missing/100000 | 5277214 | 5277337 | 121 | items_per_second=18.949M/s |
| StdSet/find_missing/500000 | 47283680 | 47284896 | 16 | items_per_second=10.5742M/s |
| StdSet/iterate/1000 | 3167 | 3167 | 215329 | items_per_second=315.742M/s |
| StdSet/iterate/10000 | 40540 | 40541 | 20606 | items_per_second=246.666M/s |
| StdSet/iterate/100000 | 428704 | 428709 | 1829 | items_per_second=233.258M/s |
| StdSet/iterate/500000 | 2732309 | 2732367 | 227 | items_per_second=182.992M/s |
| StdSet/erase_by_key/1000 | 128890 | 128907 | 5298 | items_per_second=7.75753M/s |
| StdSet/erase_by_key/10000 | 1823442 | 1823522 | 384 | items_per_second=5.48389M/s |
| StdSet/erase_by_key/100000 | 31300934 | 31301771 | 23 | items_per_second=3.19471M/s |
| StdSet/erase_by_key/500000 | 292633775 | 292635789 | 2 | items_per_second=1.70861M/s |

## vector bench

| Benchmark | Time (ns) | CPU (ns) | Iterations | UserCounters |
|-----------|-----------|----------|------------|---------------|
| BM_ConstructSize<std::vector<int>>/4 | 14.4 | 14.4 | 46451276 | items_per_second=278.014M/s |
| BM_ConstructSize<std::vector<int>>/8 | 13.7 | 13.7 | 48526094 | items_per_second=583.272M/s |
| BM_ConstructSize<std::vector<int>>/16 | 13.6 | 13.6 | 49859768 | items_per_second=1.17874G/s |
| BM_ConstructSize<std::vector<int>>/32 | 13.5 | 13.5 | 50927381 | items_per_second=2.36488G/s |
| BM_ConstructSize<std::vector<int>>/64 | 14.6 | 14.6 | 45593507 | items_per_second=4.39806G/s |
| BM_ConstructSize<std::vector<int>>/128 | 15.4 | 15.4 | 43914365 | items_per_second=8.32551G/s |
| BM_ConstructSize<std::vector<int>>/256 | 16.2 | 16.2 | 42164119 | items_per_second=15.8124G/s |
| BM_ConstructSize<std::vector<int>>/512 | 30.5 | 30.6 | 22451508 | items_per_second=16.7592G/s |
| BM_ConstructSize<std::vector<int>>/1024 | 43.0 | 43.0 | 16238049 | items_per_second=23.8346G/s |
| BM_ConstructSize<fermat::Vector<int>>/4 | 5.29 | 5.29 | 127664425 | items_per_second=756.76M/s |
| BM_ConstructSize<fermat::Vector<int>>/8 | 5.37 | 5.37 | 120814930 | items_per_second=1.48894G/s |
| BM_ConstructSize<fermat::Vector<int>>/16 | 5.38 | 5.38 | 120884227 | items_per_second=2.97547G/s |
| BM_ConstructSize<fermat::Vector<int>>/32 | 5.51 | 5.51 | 118025411 | items_per_second=5.80682G/s |
| BM_ConstructSize<fermat::Vector<int>>/64 | 7.15 | 7.15 | 88978510 | items_per_second=8.94493G/s |
| BM_ConstructSize<fermat::Vector<int>>/128 | 8.33 | 8.33 | 79113493 | items_per_second=15.3607G/s |
| BM_ConstructSize<fermat::Vector<int>>/256 | 10.2 | 10.2 | 65992612 | items_per_second=25.1554G/s |
| BM_ConstructSize<fermat::Vector<int>>/512 | 14.0 | 14.0 | 48222372 | items_per_second=36.5102G/s |
| BM_ConstructSize<fermat::Vector<int>>/1024 | 23.1 | 23.1 | 30349799 | items_per_second=44.2336G/s |
| BM_ConstructSize<turbo::InlinedVector<int,128>>/4 | 1.27 | 1.27 | 539979579 | items_per_second=3.1392G/s |
| BM_ConstructSize<turbo::InlinedVector<int,128>>/8 | 1.27 | 1.27 | 539570497 | items_per_second=6.31575G/s |
| BM_ConstructSize<turbo::InlinedVector<int,128>>/16 | 1.27 | 1.27 | 535307879 | items_per_second=12.6459G/s |
| BM_ConstructSize<turbo::InlinedVector<int,128>>/32 | 1.84 | 1.84 | 367343763 | items_per_second=17.3871G/s |
| BM_ConstructSize<turbo::InlinedVector<int,128>>/64 | 3.71 | 3.71 | 188770611 | items_per_second=17.2717G/s |
| BM_ConstructSize<turbo::InlinedVector<int,128>>/128 | 4.62 | 4.62 | 150449569 | items_per_second=27.679G/s |
| BM_ConstructSize<turbo::InlinedVector<int,128>>/256 | 16.1 | 16.1 | 41200174 | items_per_second=15.8657G/s |
| BM_ConstructSize<turbo::InlinedVector<int,128>>/512 | 28.6 | 28.6 | 23959487 | items_per_second=17.9212G/s |
| BM_ConstructSize<turbo::InlinedVector<int,128>>/1024 | 41.8 | 41.8 | 16737159 | items_per_second=24.4963G/s |
| BM_PushBackSmall<std::vector<int>>/4 | 14.5 | 14.5 | 46494122 | items_per_second=275.936M/s |
| BM_PushBackSmall<std::vector<int>>/8 | 15.5 | 15.5 | 44820178 | items_per_second=515.997M/s |
| BM_PushBackSmall<std::vector<int>>/16 | 19.2 | 19.2 | 36196530 | items_per_second=834.686M/s |
| BM_PushBackSmall<std::vector<int>>/32 | 29.9 | 29.9 | 23489956 | items_per_second=1.07088G/s |
| BM_PushBackSmall<std::vector<int>>/64 | 40.5 | 40.5 | 17422283 | items_per_second=1.58027G/s |
| BM_PushBackSmall<std::vector<int>>/128 | 63.5 | 63.5 | 10423416 | items_per_second=2.01624G/s |
| BM_PushBackSmall<std::vector<int>>/256 | 123 | 123 | 5552676 | items_per_second=2.07884G/s |
| BM_PushBackSmall<std::vector<int>>/512 | 249 | 249 | 2790244 | items_per_second=2.05573G/s |
| BM_PushBackSmall<std::vector<int>>/1024 | 484 | 484 | 1366285 | items_per_second=2.11353G/s |
| BM_PushBackSmall<fermat::Vector<int>>/4 | 6.83 | 6.83 | 99039257 | items_per_second=585.615M/s |
| BM_PushBackSmall<fermat::Vector<int>>/8 | 9.12 | 9.12 | 74933487 | items_per_second=877.495M/s |
| BM_PushBackSmall<fermat::Vector<int>>/16 | 12.9 | 12.9 | 53900540 | items_per_second=1.24476G/s |
| BM_PushBackSmall<fermat::Vector<int>>/32 | 20.6 | 20.6 | 34000713 | items_per_second=1.55368G/s |
| BM_PushBackSmall<fermat::Vector<int>>/64 | 34.1 | 34.1 | 20475837 | items_per_second=1.87875G/s |
| BM_PushBackSmall<fermat::Vector<int>>/128 | 63.7 | 63.7 | 10538215 | items_per_second=2.01076G/s |
| BM_PushBackSmall<fermat::Vector<int>>/256 | 122 | 122 | 5487848 | items_per_second=2.09177G/s |
| BM_PushBackSmall<fermat::Vector<int>>/512 | 241 | 241 | 2886550 | items_per_second=2.12864G/s |
| BM_PushBackSmall<fermat::Vector<int>>/1024 | 481 | 481 | 1352067 | items_per_second=2.12777G/s |
| BM_EmplaceBackSmall<std::vector<int>>/4 | 14.9 | 14.9 | 46189720 | items_per_second=267.609M/s |
| BM_EmplaceBackSmall<std::vector<int>>/8 | 15.8 | 15.8 | 44542328 | items_per_second=504.818M/s |
| BM_EmplaceBackSmall<std::vector<int>>/16 | 19.8 | 19.8 | 35793971 | items_per_second=808.359M/s |
| BM_EmplaceBackSmall<std::vector<int>>/32 | 29.4 | 29.4 | 23117154 | items_per_second=1.08817G/s |
| BM_EmplaceBackSmall<std::vector<int>>/64 | 40.2 | 40.2 | 17265049 | items_per_second=1.59123G/s |
| BM_EmplaceBackSmall<std::vector<int>>/128 | 63.8 | 63.8 | 10486727 | items_per_second=2.00623G/s |
| BM_EmplaceBackSmall<std::vector<int>>/256 | 122 | 122 | 5492117 | items_per_second=2.0962G/s |
| BM_EmplaceBackSmall<std::vector<int>>/512 | 250 | 250 | 2757062 | items_per_second=2.04402G/s |
| BM_EmplaceBackSmall<std::vector<int>>/1024 | 488 | 488 | 1378797 | items_per_second=2.09963G/s |
| BM_EmplaceBackSmall<fermat::Vector<int>>/4 | 7.66 | 7.66 | 90258100 | items_per_second=521.958M/s |
| BM_EmplaceBackSmall<fermat::Vector<int>>/8 | 9.54 | 9.54 | 71991422 | items_per_second=838.892M/s |
| BM_EmplaceBackSmall<fermat::Vector<int>>/16 | 13.3 | 13.3 | 54665012 | items_per_second=1.20391G/s |
| BM_EmplaceBackSmall<fermat::Vector<int>>/32 | 26.4 | 26.4 | 26322151 | items_per_second=1.21355G/s |
| BM_EmplaceBackSmall<fermat::Vector<int>>/64 | 42.0 | 42.0 | 16520539 | items_per_second=1.52308G/s |
| BM_EmplaceBackSmall<fermat::Vector<int>>/128 | 71.3 | 71.3 | 9526185 | items_per_second=1.79628G/s |
| BM_EmplaceBackSmall<fermat::Vector<int>>/256 | 130 | 130 | 5146678 | items_per_second=1.97209G/s |
| BM_EmplaceBackSmall<fermat::Vector<int>>/512 | 249 | 249 | 2793115 | items_per_second=2.0575G/s |
| BM_EmplaceBackSmall<fermat::Vector<int>>/1024 | 490 | 490 | 1356899 | items_per_second=2.09021G/s |
| BM_IterationSmall<std::vector<int>>/4 | 14.5 | 14.5 | 46752553 | items_per_second=276.057M/s |
| BM_IterationSmall<std::vector<int>>/8 | 15.4 | 15.4 | 45145370 | items_per_second=519.687M/s |
| BM_IterationSmall<std::vector<int>>/16 | 17.5 | 17.5 | 39131480 | items_per_second=911.93M/s |
| BM_IterationSmall<std::vector<int>>/32 | 22.4 | 22.4 | 31196283 | items_per_second=1.43148G/s |
| BM_IterationSmall<std::vector<int>>/64 | 31.8 | 31.8 | 21645883 | items_per_second=2.01411G/s |
| BM_IterationSmall<std::vector<int>>/128 | 48.5 | 48.5 | 13461533 | items_per_second=2.63798G/s |
| BM_IterationSmall<std::vector<int>>/256 | 96.7 | 96.7 | 6819394 | items_per_second=2.646G/s |
| BM_IterationSmall<std::vector<int>>/512 | 171 | 171 | 3755129 | items_per_second=2.99079G/s |
| BM_IterationSmall<std::vector<int>>/1024 | 291 | 291 | 2392594 | items_per_second=3.52421G/s |
| BM_IterationSmall<fermat::Vector<int>>/4 | 5.79 | 5.79 | 111110758 | items_per_second=690.805M/s |
| BM_IterationSmall<fermat::Vector<int>>/8 | 8.79 | 8.79 | 84832308 | items_per_second=910.431M/s |
| BM_IterationSmall<fermat::Vector<int>>/16 | 11.7 | 11.7 | 55189321 | items_per_second=1.36975G/s |
| BM_IterationSmall<fermat::Vector<int>>/32 | 18.7 | 18.7 | 36897025 | items_per_second=1.70934G/s |
| BM_IterationSmall<fermat::Vector<int>>/64 | 28.7 | 28.7 | 24538415 | items_per_second=2.22963G/s |
| BM_IterationSmall<fermat::Vector<int>>/128 | 46.1 | 46.1 | 15241710 | items_per_second=2.77518G/s |
| BM_IterationSmall<fermat::Vector<int>>/256 | 78.7 | 78.7 | 8385816 | items_per_second=3.2532G/s |
| BM_IterationSmall<fermat::Vector<int>>/512 | 171 | 171 | 4164405 | items_per_second=2.99606G/s |
| BM_IterationSmall<fermat::Vector<int>>/1024 | 269 | 269 | 2603266 | items_per_second=3.80595G/s |
| BM_RandomAccessSmall<std::vector<int>>/4 | 165 | 165 | 4147274 | items_per_second=604.962M/s |
| BM_RandomAccessSmall<std::vector<int>>/8 | 165 | 165 | 4171885 | items_per_second=604.464M/s |
| BM_RandomAccessSmall<std::vector<int>>/16 | 166 | 166 | 4142441 | items_per_second=602.71M/s |
| BM_RandomAccessSmall<std::vector<int>>/32 | 166 | 166 | 4164314 | items_per_second=600.889M/s |
| BM_RandomAccessSmall<std::vector<int>>/64 | 167 | 167 | 4119679 | items_per_second=600.295M/s |
| BM_RandomAccessSmall<std::vector<int>>/128 | 166 | 166 | 4167672 | items_per_second=602.469M/s |
| BM_RandomAccessSmall<std::vector<int>>/256 | 166 | 166 | 4211795 | items_per_second=601.467M/s |
| BM_RandomAccessSmall<std::vector<int>>/512 | 166 | 166 | 4207995 | items_per_second=601.729M/s |
| BM_RandomAccessSmall<std::vector<int>>/1024 | 165 | 165 | 4131296 | items_per_second=604.239M/s |
| BM_RandomAccessSmall<fermat::Vector<int>>/4 | 485 | 485 | 1537582 | items_per_second=206.001M/s |
| BM_RandomAccessSmall<fermat::Vector<int>>/8 | 321 | 321 | 2147523 | items_per_second=311.082M/s |
| BM_RandomAccessSmall<fermat::Vector<int>>/16 | 252 | 252 | 2724473 | items_per_second=396.101M/s |
| BM_RandomAccessSmall<fermat::Vector<int>>/32 | 215 | 215 | 3182937 | items_per_second=464.197M/s |
| BM_RandomAccessSmall<fermat::Vector<int>>/64 | 196 | 196 | 3495179 | items_per_second=509.929M/s |
| BM_RandomAccessSmall<fermat::Vector<int>>/128 | 188 | 188 | 3641863 | items_per_second=531.246M/s |
| BM_RandomAccessSmall<fermat::Vector<int>>/256 | 181 | 181 | 3786362 | items_per_second=552.641M/s |
| BM_RandomAccessSmall<fermat::Vector<int>>/512 | 179 | 179 | 3849487 | items_per_second=557.76M/s |
| BM_RandomAccessSmall<fermat::Vector<int>>/1024 | 179 | 179 | 3824369 | items_per_second=559.264M/s |
| BM_InsertMiddleSmall<std::vector<int>>/4 | 69.7 | 69.7 | 9469515 | items_per_second=143.4M/s |
| BM_InsertMiddleSmall<std::vector<int>>/8 | 68.6 | 68.6 | 10298554 | items_per_second=145.776M/s |
| BM_InsertMiddleSmall<std::vector<int>>/16 | 55.4 | 55.4 | 12170615 | items_per_second=180.476M/s |
| BM_InsertMiddleSmall<std::vector<int>>/32 | 57.1 | 57.1 | 11563515 | items_per_second=175.164M/s |
| BM_InsertMiddleSmall<std::vector<int>>/64 | 74.9 | 74.9 | 9153290 | items_per_second=133.426M/s |
| BM_InsertMiddleSmall<std::vector<int>>/128 | 78.4 | 78.4 | 8982479 | items_per_second=127.596M/s |
| BM_InsertMiddleSmall<std::vector<int>>/256 | 111 | 111 | 6238219 | items_per_second=90.257M/s |
| BM_InsertMiddleSmall<std::vector<int>>/512 | 153 | 153 | 4766961 | items_per_second=65.5591M/s |
| BM_InsertMiddleSmall<std::vector<int>>/1024 | 208 | 208 | 3259727 | items_per_second=48.1577M/s |
| BM_InsertMiddleSmall<fermat::Vector<int>>/4 | 40.6 | 40.6 | 17260794 | items_per_second=246.176M/s |
| BM_InsertMiddleSmall<fermat::Vector<int>>/8 | 48.5 | 48.5 | 13691679 | items_per_second=206.325M/s |
| BM_InsertMiddleSmall<fermat::Vector<int>>/16 | 52.2 | 52.2 | 12749347 | items_per_second=191.679M/s |
| BM_InsertMiddleSmall<fermat::Vector<int>>/32 | 53.1 | 53.1 | 12460619 | items_per_second=188.185M/s |
| BM_InsertMiddleSmall<fermat::Vector<int>>/64 | 72.9 | 72.9 | 9383697 | items_per_second=137.25M/s |
| BM_InsertMiddleSmall<fermat::Vector<int>>/128 | 75.1 | 75.1 | 9067672 | items_per_second=133.218M/s |
| BM_InsertMiddleSmall<fermat::Vector<int>>/256 | 89.7 | 89.7 | 7434829 | items_per_second=111.503M/s |
| BM_InsertMiddleSmall<fermat::Vector<int>>/512 | 123 | 123 | 5795307 | items_per_second=81.2987M/s |
| BM_InsertMiddleSmall<fermat::Vector<int>>/1024 | 236 | 236 | 2982145 | items_per_second=42.4158M/s |
| BM_EraseMiddleSmall<std::vector<int>>/4 | 19.7 | 19.7 | 35045783 | items_per_second=507.138M/s |
| BM_EraseMiddleSmall<std::vector<int>>/8 | 28.2 | 28.2 | 24469512 | items_per_second=354.827M/s |
| BM_EraseMiddleSmall<std::vector<int>>/16 | 44.6 | 44.6 | 15600490 | items_per_second=223.998M/s |
| BM_EraseMiddleSmall<std::vector<int>>/32 | 52.0 | 52.0 | 12825419 | items_per_second=192.309M/s |
| BM_EraseMiddleSmall<std::vector<int>>/64 | 59.5 | 59.5 | 11276642 | items_per_second=168.176M/s |
| BM_EraseMiddleSmall<std::vector<int>>/128 | 67.3 | 67.3 | 10033861 | items_per_second=148.592M/s |
| BM_EraseMiddleSmall<std::vector<int>>/256 | 87.5 | 87.5 | 7587400 | items_per_second=114.235M/s |
| BM_EraseMiddleSmall<std::vector<int>>/512 | 120 | 120 | 5761048 | items_per_second=83.242M/s |
| BM_EraseMiddleSmall<std::vector<int>>/1024 | 174 | 174 | 3936037 | items_per_second=57.4332M/s |
| BM_EraseMiddleSmall<fermat::Vector<int>>/4 | 12.6 | 12.6 | 54958294 | items_per_second=794.626M/s |
| BM_EraseMiddleSmall<fermat::Vector<int>>/8 | 23.9 | 23.9 | 28689324 | items_per_second=418.935M/s |
| BM_EraseMiddleSmall<fermat::Vector<int>>/16 | 41.0 | 41.0 | 17095815 | items_per_second=244.055M/s |
| BM_EraseMiddleSmall<fermat::Vector<int>>/32 | 52.8 | 52.8 | 12344876 | items_per_second=189.336M/s |
| BM_EraseMiddleSmall<fermat::Vector<int>>/64 | 55.8 | 55.8 | 12212508 | items_per_second=179.111M/s |
| BM_EraseMiddleSmall<fermat::Vector<int>>/128 | 66.0 | 66.0 | 9749140 | items_per_second=151.442M/s |
| BM_EraseMiddleSmall<fermat::Vector<int>>/256 | 82.4 | 82.4 | 8216384 | items_per_second=121.408M/s |
| BM_EraseMiddleSmall<fermat::Vector<int>>/512 | 106 | 106 | 6156876 | items_per_second=94.4834M/s |
| BM_EraseMiddleSmall<fermat::Vector<int>>/1024 | 156 | 156 | 4488802 | items_per_second=64.2246M/s |
| BM_ClearShrinkSmall<std::vector<int>>/4 | 13.6 | 13.6 | 48230253 | items_per_second=293.787M/s |
| BM_ClearShrinkSmall<std::vector<int>>/8 | 14.0 | 14.0 | 50555977 | items_per_second=571.859M/s |
| BM_ClearShrinkSmall<std::vector<int>>/16 | 13.4 | 13.4 | 49537699 | items_per_second=1.18962G/s |
| BM_ClearShrinkSmall<std::vector<int>>/32 | 14.5 | 14.5 | 46006647 | items_per_second=2.21307G/s |
| BM_ClearShrinkSmall<std::vector<int>>/64 | 14.3 | 14.3 | 46993400 | items_per_second=4.48495G/s |
| BM_ClearShrinkSmall<std::vector<int>>/128 | 16.9 | 16.9 | 42371021 | items_per_second=7.55969G/s |
| BM_ClearShrinkSmall<std::vector<int>>/256 | 18.4 | 18.4 | 37823861 | items_per_second=13.9263G/s |
| BM_ClearShrinkSmall<std::vector<int>>/512 | 31.4 | 31.5 | 22194733 | items_per_second=16.2793G/s |
| BM_ClearShrinkSmall<std::vector<int>>/1024 | 38.2 | 38.2 | 18329942 | items_per_second=26.78G/s |
| BM_ClearShrinkSmall<fermat::Vector<int>>/4 | 11.6 | 11.6 | 60801606 | items_per_second=345.832M/s |
| BM_ClearShrinkSmall<fermat::Vector<int>>/8 | 11.1 | 11.1 | 63359790 | items_per_second=722.996M/s |
| BM_ClearShrinkSmall<fermat::Vector<int>>/16 | 11.2 | 11.2 | 60563496 | items_per_second=1.4283G/s |
| BM_ClearShrinkSmall<fermat::Vector<int>>/32 | 11.7 | 11.7 | 58094055 | items_per_second=2.74636G/s |
| BM_ClearShrinkSmall<fermat::Vector<int>>/64 | 11.9 | 11.9 | 57523606 | items_per_second=5.38719G/s |
| BM_ClearShrinkSmall<fermat::Vector<int>>/128 | 14.2 | 14.2 | 46327081 | items_per_second=9.03221G/s |
| BM_ClearShrinkSmall<fermat::Vector<int>>/256 | 15.6 | 15.6 | 43563305 | items_per_second=16.4464G/s |
| BM_ClearShrinkSmall<fermat::Vector<int>>/512 | 20.3 | 20.3 | 33099663 | items_per_second=25.2734G/s |
| BM_ClearShrinkSmall<fermat::Vector<int>>/1024 | 27.2 | 27.2 | 25152817 | items_per_second=37.6182G/s |
| BM_PushBack<std::vector<int>>/1000 | 487 | 487 | 1366373 | items_per_second=2.05444G/s |
| BM_PushBack<std::vector<int>>/10000 | 4707 | 4708 | 148888 | items_per_second=2.12421G/s |
| BM_PushBack<std::vector<int>>/100000 | 53318 | 53321 | 12626 | items_per_second=1.87545G/s |
| BM_PushBack<fermat::Vector<int>>/1000 | 473 | 473 | 1481342 | items_per_second=2.1144G/s |
| BM_PushBack<fermat::Vector<int>>/10000 | 4728 | 4729 | 130186 | items_per_second=2.11479G/s |
| BM_PushBack<fermat::Vector<int>>/100000 | 54639 | 54642 | 11700 | items_per_second=1.83009G/s |
| BM_EmplaceBack<std::vector<int>>/1000 | 485 | 485 | 1399135 | items_per_second=2.06344G/s |
| BM_EmplaceBack<std::vector<int>>/10000 | 4712 | 4712 | 139066 | items_per_second=2.12228G/s |
| BM_EmplaceBack<std::vector<int>>/100000 | 53290 | 53294 | 12480 | items_per_second=1.87639G/s |
| BM_EmplaceBack<fermat::Vector<int>>/1000 | 474 | 474 | 1467396 | items_per_second=2.10883G/s |
| BM_EmplaceBack<fermat::Vector<int>>/10000 | 4728 | 4728 | 148395 | items_per_second=2.11513G/s |
| BM_EmplaceBack<fermat::Vector<int>>/100000 | 54942 | 54946 | 12009 | items_per_second=1.81998G/s |
| BM_Iteration<std::vector<int>>/1000 | 245 | 245 | 2779732 | items_per_second=4.088G/s |
| BM_Iteration<std::vector<int>>/10000 | 2369 | 2369 | 287427 | items_per_second=4.22173G/s |
| BM_Iteration<std::vector<int>>/100000 | 23481 | 23483 | 29265 | items_per_second=4.25848G/s |
| BM_Iteration<fermat::Vector<int>>/1000 | 249 | 249 | 2840725 | items_per_second=4.02131G/s |
| BM_Iteration<fermat::Vector<int>>/10000 | 2353 | 2353 | 276634 | items_per_second=4.24982G/s |
| BM_Iteration<fermat::Vector<int>>/100000 | 24277 | 24276 | 28268 | items_per_second=4.11923G/s |
| BM_RandomAccess<std::vector<int>>/10000 | 1703 | 1703 | 403137 | items_per_second=587.252M/s |
| BM_RandomAccess<std::vector<int>>/100000 | 1714 | 1714 | 407859 | items_per_second=583.453M/s |
| BM_RandomAccess<fermat::Vector<int>>/10000 | 1826 | 1826 | 377872 | items_per_second=547.502M/s |
| BM_RandomAccess<fermat::Vector<int>>/100000 | 2069 | 2069 | 337262 | items_per_second=483.287M/s |
| BM_InsertMiddle<std::vector<int>>/1000 | 1494 | 1494 | 464254 | items_per_second=66.9175M/s |
| BM_InsertMiddle<std::vector<int>>/10000 | 13437 | 13438 | 50540 | items_per_second=7.44173M/s |
| BM_InsertMiddle<fermat::Vector<int>>/1000 | 1540 | 1540 | 443110 | items_per_second=64.9527M/s |
| BM_InsertMiddle<fermat::Vector<int>>/10000 | 12812 | 12812 | 52879 | items_per_second=7.80489M/s |
| BM_EraseMiddle<std::vector<int>>/1000 | 1384 | 1384 | 490250 | items_per_second=72.2313M/s |
| BM_EraseMiddle<std::vector<int>>/10000 | 456926 | 456957 | 1538 | items_per_second=218.839k/s |
| BM_EraseMiddle<fermat::Vector<int>>/1000 | 1361 | 1361 | 498834 | items_per_second=73.4505M/s |
| BM_EraseMiddle<fermat::Vector<int>>/10000 | 456727 | 456753 | 1528 | items_per_second=218.936k/s |
| BM_Sort<std::vector<int>>/10000 | 333292 | 333310 | 2085 | items_per_second=30.0021M/s |
| BM_Sort<std::vector<int>>/100000 | 4516001 | 4516285 | 158 | items_per_second=22.1421M/s |
| BM_Sort<fermat::Vector<int>>/10000 | 351755 | 351771 | 1962 | items_per_second=28.4276M/s |
| BM_Sort<fermat::Vector<int>>/100000 | 4707572 | 4707785 | 150 | items_per_second=21.2414M/s |
| BM_ClearAndShrink<std::vector<int>>/10000 | 706 | 706 | 947037 | items_per_second=14.1639G/s |
| BM_ClearAndShrink<std::vector<int>>/100000 | 7214 | 7214 | 94972 | items_per_second=13.8615G/s |
| BM_ClearAndShrink<fermat::Vector<int>>/10000 | 665 | 665 | 963823 | items_per_second=15.0395G/s |
| BM_ClearAndShrink<fermat::Vector<int>>/100000 | 7117 | 7118 | 98135 | items_per_second=14.0493G/s |

## stack

以下是将您提供的基准测试结果整理成的 Markdown 表格：

| Benchmark | Time (ns) | CPU (ns) | Iterations | UserCounters |
|-----------|-----------|----------|------------|---------------|
| BM_StdStack_PushPop/1000 | 926 | 926 | 661769 | items_per_second=2.15996G/s |
| BM_StdStack_PushPop/10000 | 10706 | 10706 | 61888 | items_per_second=1.86803G/s |
| BM_StdStack_PushPop/100000 | 107606 | 107602 | 6429 | items_per_second=1.8587G/s |
| BM_StdStack_Top/1000 | 0.404 | 0.404 | 1688941114 | items_per_second=2.47341G/s |
| BM_StdStack_Top/10000 | 0.408 | 0.408 | 1673023903 | items_per_second=2.45258G/s |
| BM_StdStack_Top/100000 | 0.412 | 0.412 | 1701014958 | items_per_second=2.425G/s |
| BM_StdStack_Emplace/1000 | 597 | 597 | 1167436 | items_per_second=1.67581G/s |
| BM_StdStack_Emplace/10000 | 6705 | 6705 | 100754 | items_per_second=1.49138G/s |
| BM_StdStack_Emplace/100000 | 69445 | 69447 | 9532 | items_per_second=1.43995G/s |
| BM_StdStack_ConstructFromContainer/1000 | 223 | 223 | 3053855 | items_per_second=4.4824G/s |
| BM_StdStack_ConstructFromContainer/10000 | 2997 | 2997 | 229551 | items_per_second=3.33665G/s |
| BM_StdStack_ConstructFromContainer/100000 | 30961 | 30962 | 22941 | items_per_second=3.22974G/s |
| BM_FermatStack_PushPop/1000 | 828 | 827 | 831547 | items_per_second=2.41732G/s |
| BM_FermatStack_PushPop/10000 | 9693 | 9692 | 70230 | items_per_second=2.06358G/s |
| BM_FermatStack_PushPop/100000 | 95114 | 95109 | 7130 | items_per_second=2.10285G/s |
| BM_FermatStack_Top/1000 | 0.287 | 0.287 | 2415831257 | items_per_second=3.4895G/s |
| BM_FermatStack_Top/10000 | 0.287 | 0.287 | 2518670228 | items_per_second=3.48527G/s |
| BM_FermatStack_Top/100000 | 0.288 | 0.288 | 2450763625 | items_per_second=3.47118G/s |
| BM_FermatStack_Emplace/1000 | 553 | 553 | 1246943 | items_per_second=1.80784G/s |
| BM_FermatStack_Emplace/10000 | 6523 | 6522 | 103624 | items_per_second=1.53318G/s |
| BM_FermatStack_Emplace/100000 | 66809 | 66798 | 10196 | items_per_second=1.49706G/s |
| BM_FermatStack_ConstructFromContainer/1000 | 20.7 | 20.7 | 33566934 | items_per_second=48.2123G/s |
| BM_FermatStack_ConstructFromContainer/10000 | 690 | 690 | 973331 | items_per_second=14.4931G/s |
| BM_FermatStack_ConstructFromContainer/100000 | 7132 | 7129 | 95147 | items_per_second=14.0262G/s |
| BM_FermatStack_GetContainer/1000 | 0.238 | 0.238 | 2944586388 | items_per_second=4.21039T/s |
| BM_FermatStack_GetContainer/10000 | 0.239 | 0.239 | 2920826934 | items_per_second=41.9125T/s |
| BM_FermatStack_GetContainer/100000 | 0.236 | 0.236 | 2935055892 | items_per_second=423.284T/s |

## radix sort

| Benchmark | Time (ns) | CPU (ns) | Iterations | UserCounters |
|-----------|-----------|----------|------------|---------------|
| BM_RadixSort_Random/1000 | 5305 | 5305 | 123560 | items_per_second=188.487M/s |
| BM_RadixSort_Random/10000 | 48351 | 48352 | 11848 | items_per_second=206.816M/s |
| BM_RadixSort_Random/100000 | 527794 | 527809 | 1213 | items_per_second=189.463M/s |
| BM_RadixSort_Random/1000000 | 6620930 | 6620792 | 93 | items_per_second=151.039M/s |
| BM_RadixSort_Sorted/1000 | 5901 | 5901 | 114491 | items_per_second=169.471M/s |
| BM_RadixSort_Sorted/10000 | 89146 | 89149 | 7788 | items_per_second=112.172M/s |
| BM_RadixSort_Sorted/100000 | 1012211 | 1012200 | 677 | items_per_second=98.7947M/s |
| BM_RadixSort_Sorted/1000000 | 12559427 | 12559778 | 55 | items_per_second=79.6192M/s |
| BM_RadixSort_ReverseSorted/1000 | 6211 | 6211 | 113330 | items_per_second=161.012M/s |
| BM_RadixSort_ReverseSorted/10000 | 85984 | 85985 | 7689 | items_per_second=116.3M/s |
| BM_RadixSort_ReverseSorted/100000 | 1016088 | 1016119 | 667 | items_per_second=98.4137M/s |
| BM_RadixSort_ReverseSorted/1000000 | 12380513 | 12380959 | 53 | items_per_second=80.7692M/s |
| BM_StdSort_Random/1000 | 6267 | 6267 | 104336 | items_per_second=159.573M/s |
| BM_StdSort_Random/10000 | 358164 | 358173 | 1931 | items_per_second=27.9194M/s |
| BM_StdSort_Random/100000 | 4700108 | 4700242 | 152 | items_per_second=21.2755M/s |
| BM_StdSort_Random/1000000 | 55164073 | 55165564 | 12 | items_per_second=18.1273M/s |
| BM_StdSort_Sorted/1000 | 3525 | 3525 | 199777 | items_per_second=283.681M/s |
| BM_StdSort_Sorted/10000 | 47760 | 47761 | 14690 | items_per_second=209.375M/s |
| BM_StdSort_Sorted/100000 | 533547 | 533558 | 1246 | items_per_second=187.421M/s |
| BM_StdSort_Sorted/1000000 | 6914404 | 6914374 | 95 | items_per_second=144.626M/s |
| BM_StdSort_ReverseSorted/1000 | 3173 | 3173 | 220562 | items_per_second=315.187M/s |
| BM_StdSort_ReverseSorted/10000 | 40398 | 40399 | 17004 | items_per_second=247.532M/s |
| BM_StdSort_ReverseSorted/100000 | 447388 | 447400 | 1564 | items_per_second=223.513M/s |
| BM_StdSort_ReverseSorted/1000000 | 5438792 | 5438950 | 117 | items_per_second=183.859M/s |
| BM_RadixSort_U32_Random/1000 | 4068 | 4068 | 186818 | items_per_second=245.807M/s |
| BM_RadixSort_U32_Random/10000 | 43511 | 43488 | 16993 | items_per_second=229.95M/s |
| BM_RadixSort_U32_Random/100000 | 491399 | 491410 | 1451 | items_per_second=203.496M/s |
| BM_RadixSort_U32_Random/1000000 | 5215961 | 5216081 | 134 | items_per_second=191.715M/s |
| BM_RadixSort_U32_Sorted/1000 | 5857 | 5857 | 120788 | items_per_second=170.728M/s |
| BM_RadixSort_U32_Sorted/10000 | 55894 | 55895 | 12006 | items_per_second=178.906M/s |
| BM_RadixSort_U32_Sorted/100000 | 866421 | 866446 | 786 | items_per_second=115.414M/s |
| BM_RadixSort_U32_Sorted/1000000 | 10431517 | 10431569 | 65 | items_per_second=95.8629M/s |
| BM_RadixSort_U32_ReverseSorted/1000 | 5819 | 5819 | 119019 | items_per_second=171.85M/s |
| BM_RadixSort_U32_ReverseSorted/10000 | 53467 | 53468 | 13142 | items_per_second=187.029M/s |
| BM_RadixSort_U32_ReverseSorted/100000 | 846511 | 846527 | 813 | items_per_second=118.13M/s |
| BM_RadixSort_U32_ReverseSorted/1000000 | 10562674 | 10563055 | 64 | items_per_second=94.6696M/s |
| BM_StdSort_U32_Random/1000 | 5442 | 5442 | 117460 | items_per_second=183.743M/s |
| BM_StdSort_U32_Random/10000 | 384583 | 384591 | 1910 | items_per_second=26.0017M/s |
| BM_StdSort_U32_Random/100000 | 4660940 | 4660911 | 151 | items_per_second=21.455M/s |
| BM_StdSort_U32_Random/1000000 | 59140231 | 59140678 | 12 | items_per_second=16.9088M/s |
| BM_StdSort_U32_Sorted/1000 | 3572 | 3572 | 191705 | items_per_second=279.919M/s |
| BM_StdSort_U32_Sorted/10000 | 43637 | 43638 | 15347 | items_per_second=229.156M/s |
| BM_StdSort_U32_Sorted/100000 | 527460 | 527475 | 1302 | items_per_second=189.582M/s |
| BM_StdSort_U32_Sorted/1000000 | 6116372 | 6116531 | 105 | items_per_second=163.491M/s |
| BM_StdSort_U32_ReverseSorted/1000 | 2604 | 2604 | 268648 | items_per_second=384.036M/s |
| BM_StdSort_U32_ReverseSorted/10000 | 35114 | 35115 | 20089 | items_per_second=284.777M/s |
| BM_StdSort_U32_ReverseSorted/100000 | 389202 | 389216 | 1800 | items_per_second=256.927M/s |
| BM_StdSort_U32_ReverseSorted/1000000 | 4799332 | 4799480 | 149 | items_per_second=208.356M/s |
| BM_RadixSort_Person_Random/1000 | 25885 | 25886 | 27559 | items_per_second=38.6311M/s |
| BM_RadixSort_Person_Random/10000 | 259525 | 259532 | 2711 | items_per_second=38.5308M/s |
| BM_RadixSort_Person_Random/100000 | 2662729 | 2662777 | 253 | items_per_second=37.5548M/s |
| BM_RadixSort_Person_Random/1000000 | 52242148 | 52242147 | 12 | items_per_second=19.1416M/s |
| BM_StdSort_Person_Random/1000 | 33664 | 33665 | 20209 | items_per_second=29.704M/s |
| BM_StdSort_Person_Random/10000 | 674357 | 674373 | 994 | items_per_second=14.8286M/s |
| BM_StdSort_Person_Random/100000 | 8381161 | 8381463 | 80 | items_per_second=11.9311M/s |
| BM_StdSort_Person_Random/1000000 | 112267600 | 112270088 | 6 | items_per_second=8.90709M/s |



## resource pool

| Benchmark | Time (ns) | CPU (ns) | Iterations | UserCounters |
|-----------|-----------|----------|------------|---------------|
| BM_AllocFree\<ShardedPool\> | 14.9 | 14.9 | 45603965 | items_per_second=66.9522M/s |
| BM_GetPut\<ShardedPool\> | 17.0 | 17.0 | 42459953 | items_per_second=58.8986M/s |
| BM_MultiThread\<ShardedPool\>/1/real_time | 0.017 us (17 ns) | 0.017 us (17 ns) | 39124244 | items_per_second=57.8377M/s |
| BM_MultiThread\<ShardedPool\>/2/real_time | 0.035 us (35 ns) | 0.035 us (35 ns) | 19257420 | items_per_second=56.6914M/s |
| BM_MultiThread\<ShardedPool\>/4/real_time | 0.072 us (72 ns) | 0.072 us (72 ns) | 9312612 | items_per_second=55.2472M/s |
| BM_MultiThread\<ShardedPool\>/8/real_time | 0.170 us (170 ns) | 0.170 us (170 ns) | 3951925 | items_per_second=46.982M/s |
| BM_MultiThread\<ShardedPool\>/16/real_time | 0.438 us (438 ns) | 0.379 us (379 ns) | 5046554 | items_per_second=36.5615M/s |

## priority queue

| Benchmark | Time (ns) | CPU (ns) | Iterations | UserCounters |
|-----------|-----------|----------|------------|---------------|
| BM_StdPriorityQueue_PushPop/1000 | 37444 | 37445 | 18241 | items_per_second=53.4124M/s |
| BM_StdPriorityQueue_PushPop/10000 | 660347 | 660360 | 1036 | items_per_second=30.2865M/s |
| BM_StdPriorityQueue_PushPop/100000 | 8423885 | 8424094 | 79 | items_per_second=23.7414M/s |
| BM_StdPriorityQueue_ConstructFromIterators/1000 | 2090 | 2089 | 314367 | items_per_second=478.675M/s |
| BM_StdPriorityQueue_ConstructFromIterators/10000 | 50306 | 50307 | 13259 | items_per_second=198.778M/s |
| BM_StdPriorityQueue_ConstructFromIterators/100000 | 782380 | 782392 | 840 | items_per_second=127.813M/s |
| BM_FermatPriorityQueue_PushPop/1000 | 19001 | 19001 | 34655 | items_per_second=105.255M/s |
| BM_FermatPriorityQueue_PushPop/10000 | 570612 | 570632 | 1141 | items_per_second=35.0489M/s |
| BM_FermatPriorityQueue_PushPop/100000 | 7297907 | 7264399 | 92 | items_per_second=27.5315M/s |
| BM_FermatPriorityQueue_ConstructFromIterators/1000 | 1671 | 1671 | 415415 | items_per_second=598.294M/s |
| BM_FermatPriorityQueue_ConstructFromIterators/10000 | 37973 | 37969 | 18530 | items_per_second=263.373M/s |
| BM_FermatPriorityQueue_ConstructFromIterators/100000 | 690595 | 690610 | 965 | items_per_second=144.799M/s |
| BM_FermatPriorityQueue_ChangeRemove/1000 | 1828 | 1828 | 369339 | items_per_second=1.09426M/s |
| BM_FermatPriorityQueue_ChangeRemove/10000 | 37705 | 37706 | 19135 | items_per_second=53.0416k/s |
| BM_FermatPriorityQueue_ChangeRemove/100000 | 697128 | 697148 | 942 | items_per_second=2.86883k/s |
| BM_FermatPriorityQueue_GetContainer/1000 | 0.252 | 0.252 | 2734373622 | items_per_second=3.97123T/s |
| BM_FermatPriorityQueue_GetContainer/10000 | 0.254 | 0.254 | 2899351114 | items_per_second=39.3396T/s |
| BM_FermatPriorityQueue_GetContainer/100000 | 0.247 | 0.247 | 2829902756 | items_per_second=405.546T/s |


## object pool

| Benchmark | Time (ns) | CPU (ns) | Iterations |
|-----------|-----------|----------|------------|
| BM_ObjectPool_SingleThread | 2.02 | 2.02 | 336348105 |
| BM_ObjectPool_ConstructDestruct | 1.89 | 1.89 | 361283955 |
| BM_NewDelete_SingleThread | 11.8 | 11.8 | 59016801 |
| BM_ObjectPool_MultiThread/threads:4 | 2.14 | 2.14 | 307998308 |
| BM_ObjectPool_MultiThread/threads:8 | 2.56 | 2.56 | 238624160 |
| BM_ObjectPool_MultiThread/threads:16 | 3.91 | 3.06 | 229328208 |
| BM_NewDelete_MultiThread/threads:4 | 12.5 | 12.5 | 52851052 |
| BM_NewDelete_MultiThread/threads:8 | 17.7 | 17.7 | 30967352 |
| BM_NewDelete_MultiThread/threads:16 | 30.9 | 24.4 | 30238096 |
| BM_ObjectPool_Batch | 313 | 313 | 2234054 |
| BM_NewDelete_Batch | 2729 | 2729 | 250913 |

## cache

以下是将您提供的 `BM_TurboCache` 与 `BM_StdCache` 基准测试结果整理成的 Markdown 表格：

| Benchmark | Time (ns) | CPU (ns) | Iterations | UserCounters |
|-----------|-----------|----------|------------|---------------|
| BM_TurboCache_InsertDistinct/1000 | 40491 | 40492 | 13923 | items_per_second=24.696M/s |
| BM_TurboCache_InsertDistinct/10000 | 513738 | 513751 | 1469 | items_per_second=19.4647M/s |
| BM_TurboCache_InsertDistinct/100000 | 6820260 | 6820226 | 93 | items_per_second=14.6623M/s |
| BM_StdCache_InsertDistinct/1000 | 74057 | 74059 | 9055 | items_per_second=13.5028M/s |
| BM_StdCache_InsertDistinct/10000 | 800206 | 800214 | 815 | items_per_second=12.4967M/s |
| BM_StdCache_InsertDistinct/100000 | 9680044 | 9680078 | 64 | items_per_second=10.3305M/s |
| BM_TurboCache_LookupHighHit/1000 | 26.2 | 26.2 | 26030631 | items_per_second=38.1025M/s |
| BM_TurboCache_LookupHighHit/10000 | 29.5 | 29.5 | 23515100 | items_per_second=33.8548M/s |
| BM_TurboCache_LookupHighHit/100000 | 37.0 | 37.0 | 19202741 | items_per_second=27.0385M/s |
| BM_StdCache_LookupHighHit/1000 | 39.7 | 39.7 | 17491452 | items_per_second=25.2132M/s |
| BM_StdCache_LookupHighHit/10000 | 47.0 | 47.0 | 14864908 | items_per_second=21.2589M/s |
| BM_StdCache_LookupHighHit/100000 | 66.3 | 66.3 | 10270184 | items_per_second=15.0901M/s |
| BM_TurboCache_LookupLowHit/1000 | 67.6 | 67.6 | 9618233 | items_per_second=14.7942M/s |
| BM_TurboCache_LookupLowHit/10000 | 104 | 104 | 7202095 | items_per_second=9.65642M/s |
| BM_TurboCache_LookupLowHit/100000 | 118 | 118 | 5641865 | items_per_second=8.48899M/s |
| BM_StdCache_LookupLowHit/1000 | 99.5 | 99.5 | 6830423 | items_per_second=10.0536M/s |
| BM_StdCache_LookupLowHit/10000 | 118 | 118 | 5968314 | items_per_second=8.44337M/s |
| BM_StdCache_LookupLowHit/100000 | 124 | 124 | 5690173 | items_per_second=8.08656M/s |
| BM_TurboCache_Update/1000 | 22.7 | 22.7 | 33097130 | items_per_second=44.0866M/s |
| BM_TurboCache_Update/10000 | 23.9 | 23.9 | 29160854 | items_per_second=41.8223M/s |
| BM_TurboCache_Update/100000 | 35.1 | 35.1 | 22965946 | items_per_second=28.5101M/s |
| BM_StdCache_Update/1000 | 33.6 | 33.6 | 19646313 | items_per_second=29.7434M/s |
| BM_StdCache_Update/10000 | 39.6 | 39.6 | 18203608 | items_per_second=25.2609M/s |
| BM_StdCache_Update/100000 | 67.3 | 67.3 | 13194684 | items_per_second=14.865M/s |
| BM_TurboCache_EraseAll/1000 | 39932 | 39933 | 17566 | items_per_second=50.0841M/s |
| BM_TurboCache_EraseAll/10000 | 460046 | 460052 | 1539 | items_per_second=43.4733M/s |
| BM_TurboCache_EraseAll/100000 | 6075890 | 6075966 | 107 | items_per_second=32.9166M/s |
| BM_StdCache_EraseAll/1000 | 73221 | 73223 | 10088 | items_per_second=27.314M/s |
| BM_StdCache_EraseAll/10000 | 781948 | 781960 | 832 | items_per_second=25.5767M/s |
| BM_StdCache_EraseAll/100000 | 9420156 | 9420201 | 69 | items_per_second=21.231M/s |
| BM_TurboCache_Touch/1000 | 21.1 | 21.1 | 33052356 | items_per_second=47.32M/s |
| BM_TurboCache_Touch/10000 | 23.7 | 23.7 | 29447879 | items_per_second=42.1982M/s |
| BM_TurboCache_Touch/100000 | 31.1 | 31.1 | 23138203 | items_per_second=32.2018M/s |
| BM_StdCache_Touch/1000 | 35.3 | 35.3 | 20389873 | items_per_second=28.3536M/s |
| BM_StdCache_Touch/10000 | 39.0 | 39.0 | 18055295 | items_per_second=25.6508M/s |
| BM_StdCache_Touch/100000 | 56.7 | 56.7 | 12356403 | items_per_second=17.6348M/s |

## iobuf

以下是您提供的最新压测数据整理成的 Markdown 表格：

| Benchmark | Time (ns) | CPU (ns) | Iterations | UserCounters |
|-----------|-----------|----------|------------|---------------|
| BM_IOBuf_RandomChunked/1024 | 162 | 162 | 4381828 | bytes_per_second=5.87385Gi/s |
| BM_IOBuf_RandomChunked/10240 | 464 | 464 | 1327921 | bytes_per_second=20.5663Gi/s |
| BM_IOBuf_RandomChunked/102400 | 5021 | 5021 | 134676 | bytes_per_second=18.9944Gi/s |
| BM_IOBuf_RandomChunked/1048576 | 49639 | 49640 | 13941 | bytes_per_second=19.6729Gi/s |
| BM_IOBuf_RandomChunked/10485760 | 599910 | 599924 | 1115 | bytes_per_second=16.2781Gi/s |
| BM_IOBuf_RandomChunked/20971520 | 1679154 | 1679083 | 417 | bytes_per_second=11.6321Gi/s |
| BM_IOBuf_RandomChunked/52428800 | 5105570 | 5105665 | 130 | bytes_per_second=9.56352Gi/s |
| BM_CordBuffer_RandomChunked/1024 | 36.1 | 36.1 | 19267766 | bytes_per_second=26.4396Gi/s |
| BM_CordBuffer_RandomChunked/10240 | 168 | 168 | 4170392 | bytes_per_second=56.888Gi/s |
| BM_CordBuffer_RandomChunked/102400 | 1946 | 1945 | 356246 | bytes_per_second=49.0212Gi/s |
| BM_CordBuffer_RandomChunked/1048576 | 20628 | 20629 | 33575 | bytes_per_second=47.3397Gi/s |
| BM_CordBuffer_RandomChunked/10485760 | 303672 | 303671 | 2310 | bytes_per_second=32.1585Gi/s |
| BM_CordBuffer_RandomChunked/20971520 | 1032222 | 1032244 | 667 | bytes_per_second=18.9212Gi/s |
| BM_CordBuffer_RandomChunked/52428800 | 3520823 | 3520808 | 193 | bytes_per_second=13.8684Gi/s |
| BM_String_RandomChunked/1024 | 42.2 | 42.2 | 16506262 | bytes_per_second=22.6152Gi/s |
| BM_String_RandomChunked/10240 | 408 | 408 | 1729554 | bytes_per_second=23.3769Gi/s |
| BM_String_RandomChunked/102400 | 4413 | 4413 | 160390 | bytes_per_second=21.6116Gi/s |
| BM_String_RandomChunked/1048576 | 74212 | 74213 | 8692 | bytes_per_second=13.1589Gi/s |
| BM_String_RandomChunked/10485760 | 1119086 | 1119113 | 595 | bytes_per_second=8.72622Gi/s |
| BM_String_RandomChunked/20971520 | 4303944 | 4303931 | 159 | bytes_per_second=4.538Gi/s |
| BM_String_RandomChunked/52428800 | 23526624 | 23526461 | 24 | bytes_per_second=2.07546Gi/s |
| BM_FermatString_RandomChunked/1024 | 28.0 | 28.0 | 24829720 | bytes_per_second=34.0147Gi/s |
| BM_FermatString_RandomChunked/10240 | 224 | 224 | 3049685 | bytes_per_second=42.5053Gi/s |
| BM_FermatString_RandomChunked/102400 | 13854 | 13854 | 49668 | bytes_per_second=6.88359Gi/s |
| BM_FermatString_RandomChunked/1048576 | 3775177 | 3775165 | 194 | bytes_per_second=264.889Mi/s |
| BM_FermatString_RandomChunked/10485760 | 861743215 | 861764688 | 1 | bytes_per_second=11.6041Mi/s |
| BM_FermatString_RandomChunked/20971520 | 3425175318 | 3425222859 | 1 | bytes_per_second=5.83904Mi/s |
| BM_FermatString_RandomChunked/52428800 | 2.2270e+10 | 2.2270e+10 | 1 | bytes_per_second=2.24518Mi/s |
| BM_Buffer_RandomChunked/1024 | 24.6 | 24.6 | 28542091 | bytes_per_second=38.7643Gi/s |
| BM_Buffer_RandomChunked/10240 | 214 | 214 | 3393443 | bytes_per_second=44.6152Gi/s |
| BM_Buffer_RandomChunked/102400 | 3842 | 3842 | 172869 | bytes_per_second=24.8194Gi/s |
| BM_Buffer_RandomChunked/1048576 | 59821 | 59823 | 11454 | bytes_per_second=16.3243Gi/s |
| BM_Buffer_RandomChunked/10485760 | 1242412 | 1242453 | 599 | bytes_per_second=7.85995Gi/s |
| BM_Buffer_RandomChunked/20971520 | 3295299 | 3295371 | 221 | bytes_per_second=5.92687Gi/s |
| BM_Buffer_RandomChunked/52428800 | 7694759 | 7694927 | 85 | bytes_per_second=6.3455Gi/s |

## buffer

以下是将您提供的 `std::vector` 与 `fermat::Buffer` 对比基准测试结果整理成的 Markdown 表格：

| Benchmark | Time (ns) | CPU (ns) | Iterations | UserCounters |
|-----------|-----------|----------|------------|---------------|
| BM_ConstructSize<std::vector<int>>/4 | 14.6 | 14.6 | 46242984 | items_per_second=273.971M/s |
| BM_ConstructSize<std::vector<int>>/8 | 14.9 | 14.9 | 45406808 | items_per_second=537.53M/s |
| BM_ConstructSize<std::vector<int>>/16 | 15.6 | 15.6 | 45625572 | items_per_second=1.02491G/s |
| BM_ConstructSize<std::vector<int>>/32 | 14.8 | 14.8 | 44090570 | items_per_second=2.15837G/s |
| BM_ConstructSize<std::vector<int>>/64 | 16.0 | 16.0 | 39742768 | items_per_second=4.00488G/s |
| BM_ConstructSize<std::vector<int>>/128 | 16.7 | 16.7 | 42539426 | items_per_second=7.64966G/s |
| BM_ConstructSize<std::vector<int>>/256 | 17.7 | 17.7 | 37296518 | items_per_second=14.4971G/s |
| BM_ConstructSize<std::vector<int>>/512 | 43.0 | 43.0 | 15895821 | items_per_second=11.8959G/s |
| BM_ConstructSize<std::vector<int>>/1024 | 55.5 | 55.5 | 12218824 | items_per_second=18.4453G/s |
| BM_ConstructSize<fermat::Buffer<int>>/4 | 5.30 | 5.30 | 114517382 | items_per_second=755.039M/s |
| BM_ConstructSize<fermat::Buffer<int>>/8 | 4.91 | 4.91 | 135935860 | items_per_second=1.62832G/s |
| BM_ConstructSize<fermat::Buffer<int>>/16 | 5.21 | 5.21 | 132985656 | items_per_second=3.07363G/s |
| BM_ConstructSize<fermat::Buffer<int>>/32 | 5.13 | 5.13 | 129020923 | items_per_second=6.23259G/s |
| BM_ConstructSize<fermat::Buffer<int>>/64 | 5.99 | 5.99 | 109607575 | items_per_second=10.676G/s |
| BM_ConstructSize<fermat::Buffer<int>>/128 | 7.44 | 7.44 | 94308756 | items_per_second=17.2136G/s |
| BM_ConstructSize<fermat::Buffer<int>>/256 | 9.60 | 9.60 | 61750684 | items_per_second=26.6687G/s |
| BM_ConstructSize<fermat::Buffer<int>>/512 | 13.7 | 13.7 | 51234806 | items_per_second=37.3951G/s |
| BM_ConstructSize<fermat::Buffer<int>>/1024 | 22.1 | 22.1 | 30504814 | items_per_second=46.2621G/s |
| BM_PushBackSmall<std::vector<int>>/4 | 17.3 | 17.3 | 40669838 | items_per_second=230.91M/s |
| BM_PushBackSmall<std::vector<int>>/8 | 16.8 | 16.8 | 41292293 | items_per_second=477.107M/s |
| BM_PushBackSmall<std::vector<int>>/16 | 18.4 | 18.4 | 36051617 | items_per_second=870.597M/s |
| BM_PushBackSmall<std::vector<int>>/32 | 22.6 | 22.6 | 31930240 | items_per_second=1.41565G/s |
| BM_PushBackSmall<std::vector<int>>/64 | 43.2 | 43.2 | 16004234 | items_per_second=1.48264G/s |
| BM_PushBackSmall<std::vector<int>>/128 | 67.5 | 67.5 | 9773962 | items_per_second=1.8973G/s |
| BM_PushBackSmall<std::vector<int>>/256 | 131 | 131 | 5133645 | items_per_second=1.95662G/s |
| BM_PushBackSmall<std::vector<int>>/512 | 274 | 274 | 2354950 | items_per_second=1.86812G/s |
| BM_PushBackSmall<std::vector<int>>/1024 | 501 | 501 | 1384987 | items_per_second=2.04408G/s |
| BM_AppendBackSmall<fermat::Buffer<int>>/4 | 5.47 | 5.47 | 123016683 | items_per_second=731.73M/s |
| BM_AppendBackSmall<fermat::Buffer<int>>/8 | 7.60 | 7.60 | 89705799 | items_per_second=1.05216G/s |
| BM_AppendBackSmall<fermat::Buffer<int>>/16 | 11.3 | 11.3 | 60107791 | items_per_second=1.41532G/s |
| BM_AppendBackSmall<fermat::Buffer<int>>/32 | 19.9 | 19.9 | 34152492 | items_per_second=1.60925G/s |
| BM_AppendBackSmall<fermat::Buffer<int>>/64 | 39.5 | 39.5 | 17915487 | items_per_second=1.62101G/s |
| BM_AppendBackSmall<fermat::Buffer<int>>/128 | 71.8 | 71.8 | 10413272 | items_per_second=1.78351G/s |
| BM_AppendBackSmall<fermat::Buffer<int>>/256 | 139 | 139 | 4698841 | items_per_second=1.84379G/s |
| BM_AppendBackSmall<fermat::Buffer<int>>/512 | 266 | 266 | 2571697 | items_per_second=1.92605G/s |
| BM_AppendBackSmall<fermat::Buffer<int>>/1024 | 528 | 528 | 1389052 | items_per_second=1.93857G/s |
| BM_IterationSmall<std::vector<int>>/4 | 16.1 | 16.1 | 38524039 | items_per_second=248.145M/s |
| BM_IterationSmall<std::vector<int>>/8 | 18.5 | 18.5 | 39169429 | items_per_second=432.841M/s |
| BM_IterationSmall<std::vector<int>>/16 | 19.8 | 19.8 | 35317892 | items_per_second=806.229M/s |
| BM_IterationSmall<std::vector<int>>/32 | 24.5 | 24.5 | 26779104 | items_per_second=1.30438G/s |
| BM_IterationSmall<std::vector<int>>/64 | 33.5 | 33.5 | 19662790 | items_per_second=1.91166G/s |
| BM_IterationSmall<std::vector<int>>/128 | 50.8 | 50.8 | 12774677 | items_per_second=2.51829G/s |
| BM_IterationSmall<std::vector<int>>/256 | 92.1 | 92.1 | 7488930 | items_per_second=2.77838G/s |
| BM_IterationSmall<std::vector<int>>/512 | 190 | 190 | 3672178 | items_per_second=2.7011G/s |
| BM_IterationSmall<std::vector<int>>/1024 | 305 | 305 | 2133196 | items_per_second=3.35292G/s |
| BM_IterationSmall<fermat::Buffer<int>>/4 | 6.02 | 6.02 | 114449099 | items_per_second=664.729M/s |
| BM_IterationSmall<fermat::Buffer<int>>/8 | 8.90 | 8.90 | 74123365 | items_per_second=899.325M/s |
| BM_IterationSmall<fermat::Buffer<int>>/16 | 13.2 | 13.2 | 54072244 | items_per_second=1.2095G/s |
| BM_IterationSmall<fermat::Buffer<int>>/32 | 21.5 | 21.5 | 34791411 | items_per_second=1.48822G/s |
| BM_IterationSmall<fermat::Buffer<int>>/64 | 33.9 | 33.9 | 19170571 | items_per_second=1.88945G/s |
| BM_IterationSmall<fermat::Buffer<int>>/128 | 57.5 | 57.5 | 12379804 | items_per_second=2.22779G/s |
| BM_IterationSmall<fermat::Buffer<int>>/256 | 111 | 111 | 5915875 | items_per_second=2.29624G/s |
| BM_IterationSmall<fermat::Buffer<int>>/512 | 187 | 187 | 3561288 | items_per_second=2.73563G/s |
| BM_IterationSmall<fermat::Buffer<int>>/1024 | 368 | 368 | 1901480 | items_per_second=2.78573G/s |
| BM_RandomAccessSmall<std::vector<int>>/4 | 170 | 170 | 3928735 | items_per_second=586.967M/s |
| BM_RandomAccessSmall<std::vector<int>>/8 | 174 | 174 | 3765499 | items_per_second=575.586M/s |
| BM_RandomAccessSmall<std::vector<int>>/16 | 177 | 177 | 4008194 | items_per_second=564.152M/s |
| BM_RandomAccessSmall<std::vector<int>>/32 | 171 | 171 | 3913794 | items_per_second=583.504M/s |
| BM_RandomAccessSmall<std::vector<int>>/64 | 169 | 169 | 3861809 | items_per_second=591.279M/s |
| BM_RandomAccessSmall<std::vector<int>>/128 | 167 | 167 | 4138450 | items_per_second=599.105M/s |
| BM_RandomAccessSmall<std::vector<int>>/256 | 169 | 169 | 4105954 | items_per_second=591.44M/s |
| BM_RandomAccessSmall<std::vector<int>>/512 | 182 | 182 | 3830693 | items_per_second=548.483M/s |
| BM_RandomAccessSmall<std::vector<int>>/1024 | 179 | 179 | 3880387 | items_per_second=558.331M/s |
| BM_RandomAccessSmall<fermat::Buffer<int>>/4 | 175 | 175 | 3936306 | items_per_second=571.223M/s |
| BM_RandomAccessSmall<fermat::Buffer<int>>/8 | 187 | 187 | 3933419 | items_per_second=534.972M/s |
| BM_RandomAccessSmall<fermat::Buffer<int>>/16 | 168 | 168 | 3978268 | items_per_second=595.043M/s |
| BM_RandomAccessSmall<fermat::Buffer<int>>/32 | 167 | 167 | 4107143 | items_per_second=599.624M/s |
| BM_RandomAccessSmall<fermat::Buffer<int>>/64 | 179 | 179 | 4166644 | items_per_second=558.163M/s |
| BM_RandomAccessSmall<fermat::Buffer<int>>/128 | 180 | 180 | 3506561 | items_per_second=556.138M/s |
| BM_RandomAccessSmall<fermat::Buffer<int>>/256 | 172 | 172 | 4137491 | items_per_second=582.519M/s |
| BM_RandomAccessSmall<fermat::Buffer<int>>/512 | 173 | 173 | 3906102 | items_per_second=578.711M/s |
| BM_RandomAccessSmall<fermat::Buffer<int>>/1024 | 174 | 174 | 4190609 | items_per_second=573.7M/s |
| BM_InsertMiddleSmall<std::vector<int>>/4 | 69.9 | 69.9 | 9344597 | items_per_second=142.994M/s |
| BM_InsertMiddleSmall<std::vector<int>>/8 | 65.2 | 65.2 | 9709109 | items_per_second=153.355M/s |
| BM_InsertMiddleSmall<std::vector<int>>/16 | 54.9 | 54.9 | 12093256 | items_per_second=182.165M/s |
| BM_InsertMiddleSmall<std::vector<int>>/32 | 59.1 | 59.1 | 11665188 | items_per_second=169.273M/s |
| BM_InsertMiddleSmall<std::vector<int>>/64 | 72.3 | 72.3 | 9375515 | items_per_second=138.371M/s |
| BM_InsertMiddleSmall<std::vector<int>>/128 | 96.3 | 96.3 | 7121051 | items_per_second=103.81M/s |
| BM_InsertMiddleSmall<std::vector<int>>/256 | 108 | 108 | 6255405 | items_per_second=92.3977M/s |
| BM_InsertMiddleSmall<std::vector<int>>/512 | 167 | 167 | 4124393 | items_per_second=59.8288M/s |
| BM_InsertMiddleSmall<std::vector<int>>/1024 | 223 | 223 | 3042996 | items_per_second=44.8801M/s |
| BM_InsertMiddleSmall<fermat::Buffer<int>>/4 | 43.9 | 43.9 | 16124468 | items_per_second=227.881M/s |
| BM_InsertMiddleSmall<fermat::Buffer<int>>/8 | 48.3 | 48.3 | 13817617 | items_per_second=207.045M/s |
| BM_InsertMiddleSmall<fermat::Buffer<int>>/16 | 56.2 | 56.2 | 12024554 | items_per_second=177.924M/s |
| BM_InsertMiddleSmall<fermat::Buffer<int>>/32 | 55.8 | 55.8 | 11840183 | items_per_second=179.333M/s |
| BM_InsertMiddleSmall<fermat::Buffer<int>>/64 | 78.4 | 78.4 | 8580659 | items_per_second=127.519M/s |
| BM_InsertMiddleSmall<fermat::Buffer<int>>/128 | 78.9 | 78.9 | 9376080 | items_per_second=126.726M/s |
| BM_InsertMiddleSmall<fermat::Buffer<int>>/256 | 92.0 | 92.0 | 7373548 | items_per_second=108.639M/s |
| BM_InsertMiddleSmall<fermat::Buffer<int>>/512 | 119 | 119 | 5622622 | items_per_second=84.3096M/s |
| BM_InsertMiddleSmall<fermat::Buffer<int>>/1024 | 222 | 222 | 3153459 | items_per_second=45.13M/s |
| BM_EraseMiddleSmall<std::vector<int>>/4 | 20.1 | 20.1 | 33859560 | items_per_second=497.145M/s |
| BM_EraseMiddleSmall<std::vector<int>>/8 | 28.7 | 28.7 | 24528911 | items_per_second=347.942M/s |
| BM_EraseMiddleSmall<std::vector<int>>/16 | 45.5 | 45.5 | 15328413 | items_per_second=219.805M/s |
| BM_EraseMiddleSmall<std::vector<int>>/32 | 52.6 | 52.6 | 12290050 | items_per_second=190.02M/s |
| BM_EraseMiddleSmall<std::vector<int>>/64 | 58.5 | 58.5 | 11438072 | items_per_second=170.969M/s |
| BM_EraseMiddleSmall<std::vector<int>>/128 | 66.1 | 66.1 | 10240327 | items_per_second=151.321M/s |
| BM_EraseMiddleSmall<std::vector<int>>/256 | 86.7 | 86.7 | 7758923 | items_per_second=115.293M/s |
| BM_EraseMiddleSmall<std::vector<int>>/512 | 133 | 133 | 5157250 | items_per_second=75.3551M/s |
| BM_EraseMiddleSmall<std::vector<int>>/1024 | 180 | 180 | 3847360 | items_per_second=55.5712M/s |
| BM_EraseMiddleSmall<fermat::Buffer<int>>/4 | 12.8 | 12.8 | 54072371 | items_per_second=783.56M/s |
| BM_EraseMiddleSmall<fermat::Buffer<int>>/8 | 24.0 | 24.0 | 29187856 | items_per_second=417.056M/s |
| BM_EraseMiddleSmall<fermat::Buffer<int>>/16 | 41.5 | 41.5 | 17082140 | items_per_second=241.116M/s |
| BM_EraseMiddleSmall<fermat::Buffer<int>>/32 | 53.1 | 53.1 | 12675767 | items_per_second=188.357M/s |
| BM_EraseMiddleSmall<fermat::Buffer<int>>/64 | 55.9 | 55.9 | 12067915 | items_per_second=178.992M/s |
| BM_EraseMiddleSmall<fermat::Buffer<int>>/128 | 65.7 | 65.7 | 9978921 | items_per_second=152.097M/s |
| BM_EraseMiddleSmall<fermat::Buffer<int>>/256 | 83.5 | 83.5 | 8118369 | items_per_second=119.717M/s |
| BM_EraseMiddleSmall<fermat::Buffer<int>>/512 | 106 | 106 | 6457069 | items_per_second=93.9495M/s |
| BM_EraseMiddleSmall<fermat::Buffer<int>>/1024 | 163 | 163 | 4201677 | items_per_second=61.4069M/s |
| BM_ClearShrinkSmall<std::vector<int>>/4 | 14.4 | 14.4 | 48303843 | items_per_second=277.739M/s |
| BM_ClearShrinkSmall<std::vector<int>>/8 | 14.3 | 14.3 | 47180350 | items_per_second=558.861M/s |
| BM_ClearShrinkSmall<std::vector<int>>/16 | 13.7 | 13.7 | 48455868 | items_per_second=1.16451G/s |
| BM_ClearShrinkSmall<std::vector<int>>/32 | 15.0 | 15.0 | 45794060 | items_per_second=2.12717G/s |
| BM_ClearShrinkSmall<std::vector<int>>/64 | 15.3 | 15.3 | 44473038 | items_per_second=4.18634G/s |
| BM_ClearShrinkSmall<std::vector<int>>/128 | 17.4 | 17.4 | 39931579 | items_per_second=7.3428G/s |
| BM_ClearShrinkSmall<std::vector<int>>/256 | 18.7 | 18.7 | 37194508 | items_per_second=13.6754G/s |
| BM_ClearShrinkSmall<std::vector<int>>/512 | 39.6 | 39.6 | 17505042 | items_per_second=12.9365G/s |
| BM_ClearShrinkSmall<std::vector<int>>/1024 | 50.2 | 50.2 | 14174535 | items_per_second=20.4114G/s |
| BM_ClearShrinkSmall<fermat::Buffer<int>>/4 | 12.4 | 12.4 | 56799628 | items_per_second=321.313M/s |
| BM_ClearShrinkSmall<fermat::Buffer<int>>/8 | 11.5 | 11.5 | 59391014 | items_per_second=695.888M/s |
| BM_ClearShrinkSmall<fermat::Buffer<int>>/16 | 11.6 | 11.6 | 58647914 | items_per_second=1.37649G/s |
| BM_ClearShrinkSmall<fermat::Buffer<int>>/32 | 12.0 | 12.0 | 55151099 | items_per_second=2.67124G/s |
| BM_ClearShrinkSmall<fermat::Buffer<int>>/64 | 12.2 | 12.2 | 56119120 | items_per_second=5.25883G/s |
| BM_ClearShrinkSmall<fermat::Buffer<int>>/128 | 14.1 | 14.1 | 47835694 | items_per_second=9.06365G/s |
| BM_ClearShrinkSmall<fermat::Buffer<int>>/256 | 16.1 | 16.1 | 42992608 | items_per_second=15.9353G/s |
| BM_ClearShrinkSmall<fermat::Buffer<int>>/512 | 21.3 | 21.3 | 32379491 | items_per_second=24.0626G/s |
| BM_ClearShrinkSmall<fermat::Buffer<int>>/1024 | 31.5 | 31.5 | 22242961 | items_per_second=32.5029G/s |
| BM_PushBack<std::vector<int>>/1000 | 486 | 486 | 1356780 | items_per_second=2.0585G/s |
| BM_PushBack<std::vector<int>>/10000 | 4636 | 4636 | 148868 | items_per_second=2.1569G/s |
| BM_PushBack<std::vector<int>>/100000 | 53702 | 53703 | 12566 | items_per_second=1.86211G/s |
| BM_Append<fermat::Buffer<int>>/1000 | 467 | 467 | 1506185 | items_per_second=2.14217G/s |
| BM_Append<fermat::Buffer<int>>/10000 | 4755 | 4755 | 150236 | items_per_second=2.1029G/s |
| BM_Append<fermat::Buffer<int>>/100000 | 55644 | 55646 | 11795 | items_per_second=1.79708G/s |
| BM_Iteration<std::vector<int>>/1000 | 244 | 244 | 2805719 | items_per_second=4.10386G/s |
| BM_Iteration<std::vector<int>>/10000 | 2334 | 2334 | 300268 | items_per_second=4.28515G/s |
| BM_Iteration<std::vector<int>>/100000 | 23590 | 23586 | 29548 | items_per_second=4.23976G/s |
| BM_Iteration<fermat::Buffer<int>>/1000 | 245 | 245 | 2854195 | items_per_second=4.08393G/s |
| BM_Iteration<fermat::Buffer<int>>/10000 | 2340 | 2340 | 294117 | items_per_second=4.27263G/s |
| BM_Iteration<fermat::Buffer<int>>/100000 | 23492 | 23493 | 29329 | items_per_second=4.25664G/s |
| BM_RandomAccess<std::vector<int>>/10000 | 1702 | 1702 | 401388 | items_per_second=587.481M/s |
| BM_RandomAccess<std::vector<int>>/100000 | 1732 | 1732 | 399727 | items_per_second=577.462M/s |
| BM_RandomAccess<fermat::Buffer<int>>/10000 | 1718 | 1718 | 395651 | items_per_second=582.188M/s |
| BM_RandomAccess<fermat::Buffer<int>>/100000 | 1739 | 1739 | 387296 | items_per_second=574.948M/s |
| BM_InsertMiddle<std::vector<int>>/1000 | 1508 | 1508 | 455820 | items_per_second=66.3336M/s |
| BM_InsertMiddle<std::vector<int>>/10000 | 14061 | 14060 | 50361 | items_per_second=7.11231M/s |
| BM_InsertMiddle<fermat::Buffer<int>>/1000 | 1529 | 1529 | 455362 | items_per_second=65.3936M/s |
| BM_InsertMiddle<fermat::Buffer<int>>/10000 | 12945 | 12944 | 53271 | items_per_second=7.72569M/s |
| BM_EraseMiddle<std::vector<int>>/1000 | 1378 | 1378 | 495330 | items_per_second=72.5649M/s |
| BM_EraseMiddle<std::vector<int>>/10000 | 459152 | 459110 | 1520 | items_per_second=217.813k/s |
| BM_EraseMiddle<fermat::Buffer<int>>/1000 | 1349 | 1349 | 500899 | items_per_second=74.1467M/s |
| BM_EraseMiddle<fermat::Buffer<int>>/10000 | 458651 | 458616 | 1525 | items_per_second=218.047k/s |
| BM_Sort<std::vector<int>>/10000 | 344916 | 344885 | 2031 | items_per_second=28.9952M/s |
| BM_Sort<std::vector<int>>/100000 | 4531394 | 4530892 | 155 | items_per_second=22.0707M/s |
| BM_Sort<fermat::Buffer<int>>/10000 | 341086 | 341048 | 2040 | items_per_second=29.3214M/s |
| BM_Sort<fermat::Buffer<int>>/100000 | 4520171 | 4519649 | 153 | items_per_second=22.1256M/s |
| BM_ClearAndShrink<std::vector<int>>/10000 | 686 | 686 | 995321 | items_per_second=14.5835G/s |
| BM_ClearAndShrink<std::vector<int>>/100000 | 6865 | 6865 | 97242 | items_per_second=14.5675G/s |
| BM_ClearAndShrink<fermat::Buffer<int>>/10000 | 710 | 710 | 958453 | items_per_second=14.0871G/s |
| BM_ClearAndShrink<fermat::Buffer<int>>/100000 | 6817 | 6817 | 97602 | items_per_second=14.6695G/s |
