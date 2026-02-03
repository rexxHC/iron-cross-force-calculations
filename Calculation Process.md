# Variables Used
## User Defined Variables
gender = Athlete sex</br> height = Athlete height in meters</br> massBody = Athlete body weight</br>

## Derived Variables
L_arm = Total length of all the arm segments</br> L_upperArm = Length of the upper arm (shoulder joint to elbow)</br> L_foreArm = Length of the forearm (elbow to wrist)</br> L_hand = Length of the hand (wrist to fingertip)</br>

M_arm = Total mass of all the arm segments</br> M_upperArm = Mass of the upper arm (shoulder joint to elbow)</br> M_foreArm = Mass of the forearm (elbow to wrist)</br> M_hand = Mass of the hand (wrist to fingertip)</br>

## Concerned Variables
L_pec = Anatomical insertion distance of the pectoralis major</br> L_lat = Anatomical insertion distance of the latissimus dorsi</br>

## Major Differentiating Variables
A_pec = Insertion angle of the pectoralis major when the arms are parallel to the ground</br> A_lat = Insertion angle of the latissimus dorsi when the arms are parallel to the ground</br> PCSA_ratio = Physiological cross-sectional area ratio (area of the latissimus dorsi / area of the pectoralis major)</br></br> <b>These three variables are the genetic identifiers of ring specialists, and favorable genetics here acutely reduce the difficulty of the iron cross.</b></br>

## Physiological Derivations
<i>All calculations took gender into account when calculating values.</i></br></br>
The <b>M</b> series variables relating to the mass of the arm segments were calculated using the Zatsiorsky-Seluyanov method with predetermined B<sub>0</sub>, B<sub>1</sub>, and B<sub>2</sub> coefficients.</br></br>
The <b>L</b> series variables relating to the length of the arm segments were calculated using standard anthropometric estimates.</br></br>
The <b>A</b> series variables were declared explicitly without any derivations; the values taken were the average angles recorded in Stanford datasets.</br></br>
The PCSA_variable was also declared explicitly; the value taken was an average from standard anatomical datasets of healthy males.</br></br>
<b>N.B.</b> The PCSA ratio determines whether the athlete is chest dominant or back dominant; usually, back dominant athletes have an easier time performing the iron cross.</br></br>

## Miscellaneous Variables
arm_derivation = The angle of the arms relative to the horizontal; here we assume the athlete is holding a perfect cross, therefore this angle is always 0.</br> g = 9.81 m/s²

## Calculations
We define the arm as a rigid lever and take the shoulder joint as the axis of rotation.</br>

## Force Variables
ring_force = Force of the rings pushing on the hand</br> arm_weight = Weight of the arm pulling itself down</br> F_pec = Tension from the pectorals</br> F_lat = Tension from the latissimus</br> deltoid_total = Force of the shoulder socket pushing back against the arm bone</br>

## Torque Calculations
ring_torque = (mg / 2) * L<sub>arm</sub> * cos(arm_derivation_angle)</br> arm_weight_torque = M<sub>arm</sub> * g * L<sub>cog</sub> * cos(arm_derivation_angle)</br> net_torque = T<sub>rings</sub> - T<sub>arm</sub></br>

## Muscular Force (External) Calculations
F<sub>lat</sub> = PCSA_ratio * F<sub>pec</sub></br> torque<sub>pec</sub> = F<sub>pec</sub> * L<sub>pec</sub> * sin(A_pec)</br> torque<sub>lat</sub> = F<sub>lat</sub> * L<sub>lat</sub> * sin(A_lat)</br> total_external = torque<sub>pec</sub> + torque<sub>lat</sub></br>

## Vector Forces
joint_horizontal = Horizontal compression (muscles pull inward and socket pushes back outward) = F<sub>hor</sub> = F<sub>pec</sub>cos(A_pec) + F<sub>lat</sub>cos(A_lat)</br> joint_vertical = Vertical shear = F<sub>ver</sub> = F<sub>rings</sub> - arm_weight - F<sub>pec</sub>cos(A_pec) - F<sub>lat</sub>cos(A_lat)</br></br> Since these vectors are perpendicular, we can use the Pythagorean theorem to find out the magnitude of the total force vector.</br></br>

F<sub>total</sub> = square_root(F<sub>hor</sub><sup>2</sup> + F<sub>ver</sub><sup>2</sup>)</br>
