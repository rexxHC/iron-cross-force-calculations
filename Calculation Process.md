# Variables Used
## user defined variables
gender = athlete sex</br>
height = athlete height in meters</br>
massBody = athlete body weight</br>

## derived variables
L_arm = total length of all the arm segments</br>
L_upperArm = length of the upper arm (shoulder joint to elbow)</br>
L_foreArm = length of the forearm (elbow to wrist)</br>
L_hand = length of the hand (wrist to fingertip)</br>

M_arm = total mass of all the arm segments</br>
M_upperArm = length of the upper arm (shoulder joint to elbow)</br>
M_foreArm = length of the forearm (elbow to wrist)</br>
M_hand = length of the hand (wrist to fingertip)</br>

## concerned variables
L_pec = anatomical insertion distance of the pectoralis major</br>
L_lat = anatomical insertion of the latissimus dorsi</br>

## major differentiating variables
A_pec = insertion angle of the pectoralis major when the arms are parallel to the ground</br>
A_lat = insertion angle of the latissimus dorsi when the arms are parallel to the ground</br>
PCSA_ratio = physiological cross-sectional area ratio (area of the latissimus dorsi / area of the pectoralis major)</br></br>
<b>these three variables are the genetic identifiers of ring specialists and favorable genetics here acutely reduce the difficulty of the iron cross</b></br>

## physiological derivations
<i>all calculations took gender into account when calculating values</i></br></br>
the M series variables relating to the mass of the arm segments were calculated using the Zatsiorsky-Seluyanov method with predetermined B<sub>0</sub>, B<sub>1</sub>, and B<sub>2</sub> co-efficeients
the L series variables relating to the length of the arm segments were calculated using standard anthropometric estimates</br>
the A series variables were declared explicitly without any derivations, the values taken was the average angles recorded in different datasets</br>
the PCSA_variable was also declared explicitly, the value taken was an average from standarad anatomical datasets of healthy males</br>
<b>n.b.</b> the PCSA ratio determines whether the athlete is chest dominant or back dominant, usually back dominant athletes have an easier time performing the iron cross</br>

## miscellaneous variables
arm_derivation = the angle of the arms realtive to the horizontal, here we assume the athlete is holding a perfect cross, therefore this angle is always 0
g = 9.81 (duh..)

# Calculations
we define the arm as a rigid lever, and take the shoulder joint as the axis of rotation</br>
## force variables
ring_froce = force of the rings pushing on the hand</br>
arm_weight = weight of the arm pulling itself down</br>
F_pec = tension from the pectorals</br>
F_lat = tension from the latissimus</br>
deltoid_total = force of the shoulder socket pushing back against the arm bone</br>

## torque calculations
ring_torque = (mg / 2) * L<sub>arm</sub> * cos(arm_derivation_angle)</br>
arm_weight_torque = M<sub>arm</sub> * g * L<sub>cog</sub> * cos(arm_derivation_angle)</br>
net_torque = T<sub>rings</sub> - T<sub>arm</sub></br>

## muscular force (external) calculations
F<sub>lat</sub> = PCSA_ratio * F<sub>pec</sub></br>
torque<sub>pec</sub> = F<sub>pec</sub> * L<sub>pec</sub> * sin(A_pec)</br>
torque<sub>lat</sub> = F<sub>lat</sub> * L<sub>lat</sub> * sin(A_lat)</br>
total_external = torque<sub>pec</sub> + torque<sub>lat</sub></br>

## vector forces 
joint_horizontal = horizontal compression (muscles pull inward and socket pushes back outward) = F<sub>hor</sub> = F<sub>pec</sub>cos(A_pec) + F<sub>lat</sub>cos(A_lat)</br>
joint_vertical = vertical shear = F<sub>ver</sub> = F<sub>rings</sub> - arm_weight - F<sub>pec</sub>cos(A_pec) - F<sub>lat</sub>cos(A_lat)</br>
since these vectors are perpendicular, we can use the pythagorean theorem to find out the magnitude of the total force vector

F<sub>total</sub> = square_root(F<sub>hor</sub><sup>2</sup> + F<sub>ver</sub><sup>2</sup>)</br>




