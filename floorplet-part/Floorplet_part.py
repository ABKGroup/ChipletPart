import numpy as np
import argparse as ap

def floorplet_part(file_name, min_chiplet_area, max_chiplet_area):
    print("****************************************************************") 
    print("Floorplet part")
    print("****************************************************************")

    # Read the input file
    print("Reading the input file: ", file_name)
    print("Minimum chiplet area: ", min_chiplet_area)
    print("Maximum chiplet area: ", max_chiplet_area)

    # Read the input file
    IP_area_list = []
    with open(file_name) as f:
        content = f.read().splitlines()
    f.close()

    for line in content:
        items = line.split()
        IP_area_list.append(float(items[1]))

    chiplet_id_list = []
    chiplet_area_list = []

    # put all the small IPs into the nr chiplet
    id = 0
    IP_chiplet_list = [-1 for i in range(len(IP_area_list))]

    for i in range(len(IP_area_list)):
        if (IP_area_list[i] >= min_chiplet_area):
            chiplet_id_list.append(id)
            chiplet_area_list.append(IP_area_list[i])
            IP_chiplet_list[i] = id
            id += 1
    

    # put the small IPs into the chiplet
    chiplet_id_list.append(id)
    chiplet_area_list.append(0)

    for i in range(len(IP_area_list)):
        if (IP_area_list[i] < min_chiplet_area and chiplet_area_list[id] + IP_area_list[i] < max_chiplet_area):
            chiplet_area_list[id] += IP_area_list[i]
            IP_chiplet_list[i] = id
        elif (IP_area_list[i] < min_chiplet_area and chiplet_area_list[id] + IP_area_list[i] >= max_chiplet_area):
            id += 1
            chiplet_id_list.append(id)
            chiplet_area_list.append(IP_area_list[i])
            IP_chiplet_list[i] = id


    print("Number of chiplets: ", len(chiplet_id_list))
    print("Chiplet ID list: ", chiplet_id_list)
    print("Chiplet area list: ", chiplet_area_list)

    # Write the output file
    output_file_name = file_name + ".floorplet_part"
    print("Writing the output file: ", output_file_name)
    with open(output_file_name, "w") as f:
        f.write("block_id chiplet_id\n")
        for i in range(len(IP_area_list)):
            f.write(str(i) + " " + str(IP_chiplet_list[i]) + "\n")
    f.close()


if __name__ == "__main__":
    parser = ap.ArgumentParser(description="Floorplet part")
    parser.add_argument("-file_name", help="Input file name")
    parser.add_argument("-min_chiplet_area", help="Minimum chiplet area")
    parser.add_argument("-max_chiplet_area", help="Maximum chiplet area")
    args = parser.parse_args()
    floorplet_part(args.file_name, float(args.min_chiplet_area), float(args.max_chiplet_area))






