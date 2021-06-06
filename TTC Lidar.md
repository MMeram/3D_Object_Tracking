## Performance Evaluation 1 (TTC Lidar)

|Ref Velocity|TTC Lidar|Computation Method|
|----|----|----|
|0.640001|12.5156|Median point method|
|0.609999|12.9722|Minimum point method|
|0.630002|12.6142|Median point method|
|0.640001|12.264|Minimum point method|
|0.560002|14.091|Median point method|
|0.559998|13.9161|Minimum point method|
|0.469999|16.6894|Median point method|
|1.08|7.11572|Minimum point method|
|0.494998|15.7465|Median point method|
|0.469999|16.2511|Minimum point method|
|0.604999|12.7835|Median point method|
|0.609999|12.4213|Minimum point method|
|0.640001|11.9844|Median point method|
|0.220003|34.3404|Minimum point method|
|0.580001|13.1241|Median point method|
|0.799999|9.34376|Minimum point method|
|0.580001|13.0241|Median point method|
|0.409999|18.1318|Minimum point method|
|0.669999|11.1746|Median point method|
|0.409999|18.0318|Minimum point method|
|0.580001|12.8086|Median point method|
|1.88|**3.83244**|Minimum point method|
|0.819998|8.95978|Median point method|
|-0.669999|**-10.8537**|Minimum point method|
|0.73|9.96439|Median point method|
|0.780001|9.22307|Minimum point method|
|0.750003|9.59863|Median point method|
|0.649996|10.9678|Minimum point method|
|0.834999|8.52157|Median point method|
|0.870004|8.09422|Minimum point method|
|0.740001|9.51552|Median point method|
|2.15|**3.17535**|Minimum point method|
|0.725|9.61241|Median point method|
|-0.689998|**-9.99424**|Minimum point method|
|0.819998|8.3988|Median point method|
|0.819998|8.30978|Minimum point method|

I calculated the TTC first with the minimum lidar points and then proceed with the median points in the respective bounding boxes. As the bold values indicate, especially the minus ones, the collision should have  already happened in the past, so it's not a feasible solution to consider the minimum values but the median ones.



## Performance Evaluation 2 (TTC Camera)

