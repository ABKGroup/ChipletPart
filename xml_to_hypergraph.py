# example of a netlist xml file: 
# <netlist>
#     <net type="UCIe_standard"
#         block0="0x"
#         block1="1x"
#         bandwidth="1">
#     </net>
#     <net type="UCIe_standard"
#         block0="1x"
#         block1="2x"
#         bandwidth="1">
#     </net>
#     <net type="UCIe_standard"
#         block0="2x"
#         block1="3x"
#         bandwidth="1">
#     </net>
#     <net type="UCIe_standard"
#         block0="3x"
#         block1="0x"
#         bandwidth="1">
#     </net>
# </netlist>

# convert this xml file to a hypergraph 
# hypergraph format is 
# 4 4 
# 1 2 
# 2 3
# 3 4
# 4 1

# python script starts here 

import xml.etree.ElementTree as ET
import sys

def convertXMLtoHypergraph(netlist_file:str, block_def_file:str, io_def_file:str, output_hgraph: str) -> None:
     # read the io_def file and save a hash map of io_type and reach
    # format is
    io_type_to_reach = {}

    tree = ET.parse(io_def_file)
    root = tree.getroot()
    for io in root.findall('io'):
        io_type_to_reach[io.attrib['type']] = float(io.attrib['reach'])
    
    # read the netlist file
    tree = ET.parse(netlist_file)
    root = tree.getroot()

    # get the number of vertices and edges in the graph 
    num_edges = len(root.findall('net'))
    # number of vertices is the max of the block0 and block1 attributes

    # get the number of vertices
    num_vertices = 0
    net_array = []
    net_wt_array = []
    reach_array = []
    # create a map of vertices to index
    vertices_to_index = {}
    for net in root.findall('net'):
        #num_vertices = max(num_vertices, int(net.attrib['block0']), int(net.attrib['block1']))
        # check if the vertices are already in the dictionary
        # if it is there then get the index and add it to the net_array
        # if it is not there then add it to the dictionary and add it to the net_array
        # first get the type of the net 
        type = net.attrib['type']

        # find the reach of the net
        reach = -1.0

        if type in io_type_to_reach:
            reach = io_type_to_reach[type]

        reach_array.append(reach)

        net_wt = 0
        if net.attrib['block0'] in vertices_to_index:
            block0_index = vertices_to_index[net.attrib['block0']]
        else:
            vertices_to_index[net.attrib['block0']] = num_vertices
            block0_index = num_vertices
            num_vertices += 1

        if net.attrib['block1'] in vertices_to_index:
            block1_index = vertices_to_index[net.attrib['block1']]
        else:
            vertices_to_index[net.attrib['block1']] = num_vertices
            block1_index = num_vertices
            num_vertices += 1

        net_wt = float(net.attrib['bandwidth'])
        net_array.append([block0_index, block1_index])
        net_wt_array.append(net_wt)

   
    # read block_definitions file
    # format is 
    # Master_Crossbar_0 0.041655 0.03
    # vertex area is 0.041655 * 0.03
        
    vertex_area = {}
    f = open(block_def_file, 'r')
    for line in f:
        vertex, area, power, tech = line.split()
        vertex_area[vertex] = float(area)  

    vertex_index_to_name = {v:k for k,v in vertices_to_index.items()}
    # write out a map of vertex indices to vertex names 
    f = open(output_hgraph + '.map', 'w')
    for vertex, index in vertices_to_index.items():
        f.write(str(index+1) + ' ' + vertex + '\n')

    f.close()

    # open the output file
    f = open(output_hgraph, 'w')
    f.write(str(num_edges) + ' ' + str(num_vertices) +  ' 11 \n')

    # write the edges to the file
    edge_idx = 0
    for edge in net_array:
        f.write(str(net_wt_array[edge_idx]) + ' ' + str(reach_array[edge_idx]) + ' ')
        for vertex in edge:
            f.write(str(vertex+1) + ' ')
        f.write('\n')

    # now write the vertex weights to the file
    for vertex in range(num_vertices):
        vertex_name = vertex_index_to_name[vertex]
        f.write(str(vertex_area[vertex_name]) + '\n')

    # close the file
    f.close()

if __name__ == '__main__':
    if len(sys.argv) != 5:
        print('Usage: python xml_to_hypergraph.py <xml_file> <block_def_file> <io_def_file> <hgraph_file>')
        sys.exit(1)
    xml_file = sys.argv[1]
    block_def_file = sys.argv[2]
    io_def_file = sys.argv[3]
    hgraph_file = sys.argv[4]
    tree = ET.parse(xml_file)
    root = tree.getroot()
    convertXMLtoHypergraph(xml_file, block_def_file, io_def_file, hgraph_file)