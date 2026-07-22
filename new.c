#include <math.h>
#include <stdio.h>

/* ============================================================================
 * DATA MODEL
 *
 * Every skill in this file reduces to the same shape of problem: a rigid
 * body segment is held static against gravity, pivoting about some joint,
 * and one or two muscles crossing that joint have to produce enough torque
 * to cancel the torque gravity is applying. Once you have:
 *   1) how heavy each body segment is,
 *   2) how long each segment is and where its center of gravity sits,
 *   3) where the muscles insert and at what angle,
 * you can compute the muscle forces and the resulting joint reaction force
 * for any static hold. The structs below hold exactly those three
 * categories of data, and are shared across all four skills.
 * ==========================================================================*/

// Raw inputs collected from the user. Gender is used because the mass and
// length regression coefficients below (Zatsiorsky-Seluyanov, de Leva) were
// fit separately for male/female cadavers/DXA scans and are not the same
// curve scaled -- hence every regression function below branches on it.
typedef struct {
  int gender;     // 1 = male, 2 = female
  float height;   // meters
  float massBody; // kg
} athlete;

// Geometry of one arm, decomposed into its three rigid segments (upper arm,
// forearm, hand) plus two derived values: total arm length, and where the
// arm's own center of gravity (L_cg) sits, measured as a distance from the
// shoulder. L_cg is needed anywhere the arm's own weight creates torque
// about the shoulder (iron cross), and isn't needed where the arm is just
// a load-bearing strut (maltese/planche use M_arm instead, see below).
typedef struct {
  float L_upperArm;
  float L_forearm;
  float L_hand;
  float L_arm; // L_upperArm + L_forearm + L_hand
  float L_cg;  // arm's center of gravity, distance from the shoulder
} length_arm;

// Mass of the same three arm segments, plus their sum. M_arm is what gets
// used as "strut weight" in skills where the arm's own torque contribution
// isn't relevant but its weight still has to be supported through the
// shoulder joint (maltese, planche).
typedef struct {
  float M_upperArm;
  float M_foreArm;
  float M_hand;
  float M_arm; // M_upperArm + M_foreArm + M_hand
} mass_arm;

// Trunk + both legs treated as a single lever, used whenever the body
// (rather than the arm) is what's cantilevered out horizontally -- maltese
// cross and planche both hang/push the whole torso+legs off the shoulder,
// so they need this instead of length_arm/mass_arm for their torque source.
// Head is deliberately left out -- see the comment on build_lower_body.
typedef struct {
  float M_trunk;
  float M_thigh; // BOTH legs combined, not per-leg -- see build_lower_body
  float M_shank; // both legs combined
  float M_foot;  // both legs combined
  float M_lever; // M_trunk + M_thigh + M_shank + M_foot
  float L_trunk;
  float L_thigh;
  float L_shank;
  float L_foot;
  float L_cg; // trunk+legs center of gravity, distance from the shoulder
} lower_body;

// A joint is generally stabilized by more than one muscle pulling at
// different angles and insertion points. This struct models exactly two of
// them sharing the load, which is enough to solve the torque equation (one
// equation, and the second muscle's force is pinned to the first via a
// fixed ratio rather than being a second free unknown). The same struct
// shape is reused for three completely different muscle pairs in this file
// (pec/lat, deltoid/serratus, hip flexors) because the underlying algebra
// is identical -- see solve_joint below for why.
typedef struct {
  float L1, A1; // primary muscle: insertion length (m), insertion angle (rad)
  float L2, A2; // secondary muscle: same, but F2 is defined relative to F1
  float ratio;  // F2 = ratio * F1 (e.g. PCSA_ratio for pec/lat)
} muscle_pair;

// Output of one torque-balance + force-balance solve at a single joint.
// This is intentionally generic (not "pec_force"/"lat_force" etc.) so the
// exact same struct can represent a shoulder result or a hip result.
typedef struct {
  float support_force;    // external reaction force at the grip/contact point
  float external_torque;  // torque gravity is applying that the muscles must
                          // cancel
  float F1, F2;           // resulting muscle forces (primary, secondary)
  float joint_horizontal; // horizontal component of the joint reaction force
  float joint_vertical;   // vertical component of the joint reaction force
  float joint_reaction_total; // magnitude of the combined reaction force
} joint_result;

// Victorian cross needs two independent joint solves (shoulder AND hip),
// so its result is just a pair of joint_results rather than a new shape.
typedef struct {
  joint_result shoulder; // pec/lat, arms held out in the cross position
  joint_result hip;      // hip flexors, holding the legs horizontal
} victorian_result;

/* ============================================================================
 * USER INPUT
 * ==========================================================================*/

// Loops until a valid 1/2 answer is given. Returned value feeds directly
// into every regression function's gender branch below.
int getGender(void) {
  while (1) {
    int n;
    printf("Select Gender\n1.Male\n2.Female\n->");
    scanf("%d", &n);
    if (n == 1 || n == 2)
      return n;
    printf("invalid choice\n");
  }
}

// Which of the four static holds to compute. Determines which structs get
// built and which compute_* function main() calls.
int getSkill(void) {
  while (1) {
    int n;
    printf(
        "Select Skill\n1.Iron Cross\n2.Maltese Cross\n3.Planche\n4.Victorian "
        "Cross\n->");
    scanf("%d", &n);
    if (n >= 1 && n <= 4)
      return n;
    printf("invalid choice\n");
  }
}

float getHeight(void) {
  float n;
  printf("Enter Height (meters): ");
  scanf("%f", &n);
  return n;
}

float getWeight(void) {
  float n;
  printf("Enter Weight (kgs): ");
  scanf("%f", &n);
  return n;
}

/* ============================================================================
 * WEIGHTED CENTER OF GRAVITY
 *
 * A multi-segment limb's overall center of gravity is the mass-weighted
 * average of each segment's own center of gravity position:
 *
 *     L_cg = sum(mass_i * dist_i) / sum(mass_i)
 *
 * where dist_i is segment i's own CoG measured from whatever pivot point
 * the caller cares about (shoulder, hip, wrist -- it doesn't matter, the
 * math is identical, only the input distances change). This single
 * function replaces what used to be separately hand-written for the arm
 * and for the trunk+legs -- every L_cg in this file now funnels through it.
 * ==========================================================================*/

float weighted_cog(int n, const float *mass, const float *dist) {
  float m_total = 0, moment = 0;
  for (int i = 0; i < n; i++) {
    m_total += mass[i];          // total mass of the chain
    moment += mass[i] * dist[i]; // sum of each segment's moment about the pivot
  }
  return moment / m_total; // moment / mass = center of gravity distance
}

/* ============================================================================
 * ARM SEGMENT MASS REGRESSIONS (Zatsiorsky-Seluyanov)
 *
 * These are linear regressions of the form:
 *     mass_segment = b0 + b1*bodyWeight(kg) + b2*bodyHeight(cm)
 * fit from cadaver/DXA data, predicting a body segment's mass from a
 * person's overall height and weight. Coefficients differ by segment (upper
 * arm vs forearm vs hand) and by gender, which is why each function below
 * has its own b0/b1/b2 pair per branch.
 * ==========================================================================*/

float upperArm_M(int gender, float weight, float height) {
  height *= 100; // regression expects height in cm, we store it in meters
  float b0, b1, b2;
  if (gender == 1) {
    b0 = -5.40;
    b1 = 0.0336;
    b2 = 0.0242;
  } else {
    b0 = -3.90;
    b1 = 0.0363;
    b2 = 0.0162;
  }
  return b0 + (b1 * weight) + (b2 * height);
}

float foreArm_M(int gender, float weight, float height) {
  height *= 100;
  float b0, b1, b2;
  if (gender == 1) {
    b0 = 0.86;
    b1 = 0.0123;
    b2 = -0.0051;
  } else {
    b0 = 0.28;
    b1 = 0.0095;
    b2 = -0.0011;
  }
  return b0 + (b1 * weight) + (b2 * height); // Zatsiorsky
}

float hand_M(int gender, float weight, float height) {
  height *= 100;
  float b0, b1, b2;
  if (gender == 1) {
    b0 = -0.27;
    b1 = 0.0055;
    b2 = 0.0023;
  } else {
    b0 = -0.24;
    b1 = 0.0039;
    b2 = 0.0028;
  }
  return b0 + (b1 * weight) + (b2 * height);
}

/* ----------------------------------------------------------------------------
 * ARM SEGMENT LENGTH REGRESSIONS
 *
 * Simpler than the mass regressions: each segment's length is just a fixed
 * fraction of total body height (a standard anthropometric approximation),
 * with a slightly different fraction for each gender.
 * --------------------------------------------------------------------------*/

float upperArm_L(int gender, float height) {
  return height * (gender == 1 ? 0.186f : 0.182f);
}

float foreArm_L(int gender, float height) {
  return height * (gender == 1 ? 0.146f : 0.143f);
}

float hand_L(int gender, float height) {
  return height * (gender == 1 ? 0.108f : 0.106f);
}

// Builds the arm's mass breakdown: three segment masses plus their sum.
// Runs the three regressions above and stores the results.
mass_arm build_mass_arm(int gender, float weight, float height) {
  mass_arm ma;
  ma.M_upperArm = upperArm_M(gender, weight, height);
  ma.M_foreArm = foreArm_M(gender, weight, height);
  ma.M_hand = hand_M(gender, weight, height);
  ma.M_arm = ma.M_upperArm + ma.M_foreArm + ma.M_hand;
  return ma;
}

// Builds the arm's length breakdown and its overall center of gravity.
// The distances passed into weighted_cog are each segment's own CoG,
// measured from the SHOULDER (the proximal end of the whole chain):
//   - upper arm's CoG sits 43.6% of the way down the upper arm from the
//     shoulder (proximal-referenced, since shoulder is upper arm's own
//     proximal joint)
//   - forearm's CoG sits 43.0% of the way down the forearm from the elbow,
//     so from the shoulder that's the full upper arm length plus that
//     fraction of the forearm
//   - hand's CoG sits 50.6% of the way down the hand from the wrist, so
//     from the shoulder that's upper arm + forearm + that fraction of hand
// These 0.436/0.430/0.506 fractions are standard segment CoG ratios from
// anthropometric tables (Winter / Dempster-style), not something derived
// elsewhere in this file.
length_arm build_length_arm(int gender, float height, mass_arm *ma) {
  length_arm la;
  la.L_upperArm = upperArm_L(gender, height);
  la.L_forearm = foreArm_L(gender, height);
  la.L_hand = hand_L(gender, height);
  la.L_arm = la.L_upperArm + la.L_forearm + la.L_hand;

  float mass[] = {ma->M_upperArm, ma->M_foreArm, ma->M_hand};
  float dist[] = {
      la.L_upperArm * 0.436f,
      la.L_upperArm + (la.L_forearm * 0.430f),
      la.L_upperArm + la.L_forearm + (la.L_hand * 0.506f),
  };
  la.L_cg = weighted_cog(3, mass, dist);
  return la;
}

/* ============================================================================
 * TRUNK + LEG REGRESSIONS (de Leva / Zatsiorsky mass fractions)
 *
 * Unlike the arm, trunk and leg masses here are modeled as fixed fractions
 * of total body weight rather than a height/weight regression -- this is a
 * coarser approximation but a standard one for these segments. Same idea
 * for lengths: fixed fractions of total body height.
 *
 * IMPORTANT: M_thigh/M_shank/M_foot are for BOTH legs combined (the 0.100,
 * 0.0465, 0.0145 literature fractions are per single leg -- doubled here),
 * because in every skill this struct is used for, both legs sit on the same
 * horizontal lever line and their weight adds directly.
 *
 * Head is left out entirely. It's ~8% of body mass, but its lever position
 * depends on how much the neck is tucked/extended, which none of these four
 * skills pin down consistently -- including it with a fixed assumed
 * position would be a bigger source of error than leaving it out.
 * ==========================================================================*/

lower_body build_lower_body(float weight, float height) {
  lower_body lb;
  lb.M_trunk = 0.497f * weight;
  lb.M_thigh = 2.0f * 0.100f * weight;  // x2: both legs
  lb.M_shank = 2.0f * 0.0465f * weight; // x2: both legs
  lb.M_foot = 2.0f * 0.0145f * weight;  // x2: both legs
  lb.M_lever = lb.M_trunk + lb.M_thigh + lb.M_shank + lb.M_foot;

  lb.L_trunk = height * 0.288f; // shoulder to hip
  lb.L_thigh = height * 0.245f; // hip to knee
  lb.L_shank = height * 0.246f; // knee to ankle
  lb.L_foot = height * 0.152f;  // ankle to toe

  // Same weighted-CoG pattern as the arm, but a 4-link chain instead of 3,
  // and measured from the shoulder instead of from the shoulder-to-hand
  // chain's proximal end. Each segment's own CoG fraction (0.50 for trunk
  // and foot, 0.433 for thigh/shank) is again a standard anthropometric
  // proximal-referenced value.
  float mass[] = {lb.M_trunk, lb.M_thigh, lb.M_shank, lb.M_foot};
  float dist[] = {
      lb.L_trunk * 0.50f,
      lb.L_trunk + (lb.L_thigh * 0.433f),
      lb.L_trunk + lb.L_thigh + (lb.L_shank * 0.433f),
      lb.L_trunk + lb.L_thigh + lb.L_shank + (lb.L_foot * 0.50f),
  };
  lb.L_cg = weighted_cog(4, mass, dist);
  return lb;
}

// Same idea as build_lower_body's L_cg, but restricted to just the legs
// (thigh+shank+foot) and measured from the HIP instead of the shoulder.
// This is only used by victorian cross: there, the hip -- not the shoulder
// -- is the pivot the legs are held against, and the trunk above the hip
// hangs as dead weight that doesn't create hip-flexion torque, so it's
// excluded from this particular CoG calculation on purpose.
float leg_cog_from_hip(lower_body *lb) {
  float mass[] = {lb->M_thigh, lb->M_shank, lb->M_foot};
  float dist[] = {
      lb->L_thigh * 0.433f,
      lb->L_thigh + (lb->L_shank * 0.433f),
      lb->L_thigh + lb->L_shank + (lb->L_foot * 0.50f),
  };
  return weighted_cog(3, mass, dist);
}

/* ============================================================================
 * MUSCLE PAIR BUILDERS
 *
 * Each of these packages up the geometry (insertion length + angle) and
 * relative strength (ratio) of two muscles sharing one joint's load, so
 * solve_joint() below has everything it needs in one struct.
 * ==========================================================================*/

// Pectoralis and latissimus insertion distances, modeled as fixed fractions
// of upper arm length -- a simplification, since real insertion points
// depend on individual shoulder anatomy, not just arm length.
float pec_ins(float L_upper) { return L_upper * 0.175f; }
float lat_ins(float L_upper) { return L_upper * 0.140f; }

// Pec + lat: the muscle pair resisting shoulder ADDUCTION torque (pulling
// the arm down/in toward the body) -- used by iron cross and maltese, where
// the arms are being held out against gravity trying to pull them down.
// Insertion angles (20°/15°) and PCSA_ratio (0.73, the ratio of
// physiological cross-sectional area between lat and pec, used here as a
// proxy for their relative force contribution) come from anthropometric/
// anatomical literature, not derived elsewhere in this file.
muscle_pair build_pec_lat(float L_upperArm) {
  muscle_pair mp;
  mp.L1 = pec_ins(L_upperArm);
  mp.A1 = 20 * (float)(M_PI / 180.0); // degrees -> radians
  mp.L2 = lat_ins(L_upperArm);
  mp.A2 = 15 * (float)(M_PI / 180.0);
  mp.ratio = 0.73f; // PCSA_ratio: lat / pec
  return mp;
}

// Anterior deltoid + serratus anterior: the pair resisting shoulder
// FLEXION / scapular protraction torque -- used by planche, where the body
// is trying to rotate the shoulder forward/down rather than pull the arm
// in toward the torso, so a different muscle group does the resisting.
//
// UNSOURCED: unlike build_pec_lat's numbers, these insertion
// lengths/angles/ratio are placeholders chosen to keep the model
// structurally consistent, not pulled from an anatomical reference. Treat
// any planche output as illustrative until these are replaced with real
// figures.
muscle_pair build_delt_serratus(float L_upperArm) {
  muscle_pair mp;
  mp.L1 = L_upperArm * 0.15f;
  mp.A1 = 15 * (float)(M_PI / 180.0);
  mp.L2 = L_upperArm * 0.10f;
  mp.A2 = 25 * (float)(M_PI / 180.0);
  mp.ratio = 0.60f;
  return mp;
}

// Iliopsoas + rectus femoris: the pair resisting hip FLEXION torque when
// the legs are being held up/horizontal against gravity (used by
// victorian's hip calculation). Same caveat as build_delt_serratus --
// placeholder geometry, not sourced from a table.
muscle_pair build_hip_flexors(float L_thigh) {
  muscle_pair mp;
  mp.L1 = L_thigh * 0.08f;
  mp.A1 = 30 * (float)(M_PI / 180.0);
  mp.L2 = L_thigh * 0.05f;
  mp.A2 = 20 * (float)(M_PI / 180.0);
  mp.ratio = 0.50f;
  return mp;
}

/* ============================================================================
 * SHARED JOINT FORCE BALANCE
 *
 * This is the core physics of every skill in this file, factored out once.
 * Given:
 *   - support_force: the external force pushing/pulling on the limb at its
 *     far end (e.g. a ring pushing back on the hand)
 *   - strut_weight: the weight of whatever segment sits between the joint
 *     being solved and that support point, which loads the joint directly
 *     via gravity regardless of any torque (e.g. the arm's own weight
 *     hanging off the shoulder). Pass 0 if there's no such segment (e.g.
 *     the hip joint in victorian has nothing "in between").
 *   - external_torque: the net torque gravity is applying about the joint,
 *     which the muscles must exactly cancel for the hold to be static
 *   - mp: the two muscles sharing the job of producing that canceling torque
 *
 * TORQUE BALANCE (solving for muscle force):
 * Each muscle produces torque = force * insertion_length * sin(angle) --
 * insertion_length is the muscle's moment arm, and sin(angle) projects the
 * muscle's pull onto the direction perpendicular to that moment arm (the
 * only component that contributes torque). Setting F2 = ratio * F1 (from
 * the muscle_pair) and requiring:
 *
 *     external_torque = F1*L1*sin(A1) + F2*L2*sin(A2)
 *                      = F1*L1*sin(A1) + (ratio*F1)*L2*sin(A2)
 *                      = F1 * [ L1*sin(A1) + ratio*L2*sin(A2) ]
 *
 * and solving for F1 gives the division below. F2 then falls straight out
 * from the ratio.
 *
 * FORCE BALANCE (solving for the joint reaction):
 * With both muscle forces known, the joint itself (the bone-on-bone contact,
 * e.g. the humeral head in the shoulder socket) has to supply whatever
 * force is left over so the whole limb is in equilibrium:
 *   - horizontally: the joint pushes back against the horizontal pull of
 *     both muscles (F*cos(angle) is each muscle's horizontal component)
 *   - vertically: support_force pushes/pulls one way, strut_weight and the
 *     vertical components of both muscles pull the other way, and whatever
 *     doesn't cancel is carried by the joint
 * The magnitude of that combined reaction is just Pythagoras.
 * ==========================================================================*/

joint_result solve_joint(float support_force, float strut_weight,
                         float external_torque, muscle_pair *mp) {
  joint_result r;
  r.support_force = support_force;
  r.external_torque = external_torque;

  // F1 solved from the torque balance equation derived above
  r.F1 = external_torque /
         ((mp->L1 * sinf(mp->A1)) + (mp->ratio * mp->L2 * sinf(mp->A2)));
  r.F2 = r.F1 * mp->ratio; // F2 is pinned to F1 by the fixed ratio

  // Joint reaction force balance: horizontal component is purely the two
  // muscles pulling inward, since support_force/strut_weight are vertical.
  r.joint_horizontal = (r.F1 * cosf(mp->A1)) + (r.F2 * cosf(mp->A2));

  // Vertical component: whatever the support force and gravity don't
  // already cancel out is what the joint itself has to carry.
  r.joint_vertical = support_force - strut_weight - (r.F1 * sinf(mp->A1)) -
                     (r.F2 * sinf(mp->A2));

  // Combined magnitude of the joint reaction force (Pythagorean theorem,
  // since horizontal and vertical components are perpendicular).
  r.joint_reaction_total = sqrtf(r.joint_horizontal * r.joint_horizontal +
                                 r.joint_vertical * r.joint_vertical);
  return r;
}

/* ============================================================================
 * IRON CROSS
 *
 * Position: arms held straight out to the sides, torso hanging straight
 * down directly below the shoulders. The ARM is the lever here -- it's held
 * out horizontally against gravity, and the ring pushes back up on the hand
 * to keep it there. The torso does NOT contribute torque about the shoulder
 * in this model, because it hangs collinear with (directly below) the
 * shoulder joint -- its weight passes straight through the pivot, giving it
 * zero moment arm.
 * ==========================================================================*/

joint_result compute_iron_cross(athlete *a, mass_arm *ma, length_arm *la,
                                muscle_pair *mp) {
  float g = 9.81f;

  // Each ring supports half of total bodyweight (both rings together
  // support the whole hanging body symmetrically).
  float ring_force = (a->massBody * g) / 2.0f;

  // The arm's own weight, hanging off the shoulder.
  float arm_weight = ma->M_arm * g;

  // Torque the ring's push creates about the shoulder, trying to rotate
  // the arm downward (this is the torque the muscles must resist).
  float ring_torque = ring_force * la->L_arm;

  // The arm's own weight also creates torque about the shoulder, in the
  // SAME rotational direction as gravity pulling the arm down -- so it's
  // subtracted from ring_torque because the ring is fighting both the
  // arm's weight and the ring's own leverage simultaneously; only the
  // portion of ring_torque beyond what's needed to also lift the arm's own
  // weight is "extra" net torque requiring muscle force. (Equivalently:
  // net external torque = ring_torque - arm_weight_torque.)
  float arm_weight_torque = arm_weight * la->L_cg;

  float external_torque = ring_torque - arm_weight_torque;

  return solve_joint(ring_force, arm_weight, external_torque, mp);
}

/* ============================================================================
 * MALTESE CROSS
 *
 * Position: arms held straight out to the sides same as iron cross, but the
 * body is horizontal -- so instead of the arm being the lever, the TRUNK+
 * LEGS are now the lever, cantilevered out from the shoulder, and the arm
 * has become a (roughly) vertical strut transmitting force from shoulder
 * down to the ring.
 *
 * ASSUMPTION: arms are held vertically, in line with the ring's reaction
 * force. Under that assumption the arm carries pure axial (compressive/
 * tensile) load and contributes ~0 torque about the shoulder -- same
 * "collinear with the pivot" logic as the torso in iron cross, just
 * applied to the arm instead. If the arms deviate meaningfully from
 * vertical this assumption breaks and an arm torque term would need to be
 * added back in.
 * ==========================================================================*/

joint_result compute_maltese_cross(athlete *a, mass_arm *ma, lower_body *lb,
                                   muscle_pair *mp) {
  float g = 9.81f;

  float ring_force =
      (a->massBody * g) / 2.0f;     // same half-bodyweight-per-ring logic
  float arm_weight = ma->M_arm * g; // arm still hangs off the shoulder

  // Only the trunk+legs lever creates torque here (per the vertical-arm
  // assumption above) -- no subtraction needed because nothing else on
  // this lever opposes gravity the way the ring did in iron cross.
  float external_torque = lb->M_lever * g * lb->L_cg;

  return solve_joint(ring_force, arm_weight, external_torque, mp);
}

/* ============================================================================
 * PLANCHE
 *
 * Position: hands on the ground, arms straight, body horizontal, cantilevered
 * forward off the shoulders -- geometrically the SAME lever/strut setup as
 * maltese cross (trunk+legs are the lever, arm is the strut). What's
 * different is which direction the shoulder is being rotated, and
 * therefore which muscles are doing the resisting: maltese pulls the arm
 * toward the body (adduction, pec/lat), planche pulls the shoulder forward
 * into flexion/protraction (deltoid/serratus). Because solve_joint only
 * cares about the muscle_pair it's handed, this function's body is
 * identical to compute_maltese_cross -- the distinction lives entirely in
 * which muscle_pair main() constructs and passes in.
 * ==========================================================================*/

joint_result compute_planche(athlete *a, mass_arm *ma, lower_body *lb,
                             muscle_pair *mp) {
  float g = 9.81f;

  float hand_force =
      (a->massBody * g) / 2.0f;     // ground reaction split across both hands
  float arm_weight = ma->M_arm * g; // arm as strut, same role as in maltese
  float external_torque =
      lb->M_lever * g * lb->L_cg; // trunk+legs cantilevered off the shoulder

  return solve_joint(hand_force, arm_weight, external_torque, mp);
}

/* ============================================================================
 * VICTORIAN CROSS
 *
 * NOT a sourced skill breakdown -- "Victorian cross" doesn't have an
 * established biomechanical model to work from, so this is a deliberate
 * modeling choice rather than a derived one:
 *
 *   - Shoulder: body inverted, arms still held out in the cross position.
 *     The arm's own torque contribution about the shoulder doesn't change
 *     with the body's orientation (gravity still pulls the arm the same
 *     way relative to itself), so this reuses the exact same shoulder
 *     torque equation as compute_iron_cross rather than calling it, since
 *     it needs the raw torque value (not a full joint_result) before
 *     solving.
 *   - Hip: unlike iron cross, the legs here are actively held horizontal
 *     against gravity rather than hanging passively, which is new work the
 *     hip flexors have to do that no other skill in this file requires.
 *     The torso above the hip is assumed to hang as dead weight and not
 *     load the hip flexors, so only leg mass (via leg_cog_from_hip)
 *     contributes to this torque. support_force and strut_weight are both
 *     0 here because the hip is an internal joint with no external contact
 *     force and nothing "in between" it and a support point.
 *
 * If your intended Victorian geometry differs, the hip block is the piece
 * to change -- the shoulder block should hold regardless.
 * ==========================================================================*/

victorian_result compute_victorian_cross(athlete *a, mass_arm *ma,
                                         length_arm *la, lower_body *lb,
                                         muscle_pair *shoulder_mp,
                                         muscle_pair *hip_mp) {
  victorian_result v;
  float g = 9.81f;

  // --- shoulder: same torque equation as compute_iron_cross ---
  float ring_force = (a->massBody * g) / 2.0f;
  float arm_weight = ma->M_arm * g;
  float shoulder_torque = (ring_force * la->L_arm) - (arm_weight * la->L_cg);
  v.shoulder =
      solve_joint(ring_force, arm_weight, shoulder_torque, shoulder_mp);

  // --- hip: legs held horizontal, torso excluded (see comment above) ---
  float M_legs = lb->M_thigh + lb->M_shank + lb->M_foot;
  float leg_cg = leg_cog_from_hip(lb);
  float hip_torque = M_legs * g * leg_cg;
  v.hip = solve_joint(0.0f, 0.0f, hip_torque, hip_mp);

  return v;
}

/* ============================================================================
 * OUTPUT
 * ==========================================================================*/

void print_joint_result(const char *label, joint_result *r) {
  printf("\n--- %s ---\n", label);
  printf("Support Force: %.2f N\n", r->support_force);
  printf("External Torque: %.2f Nm\n", r->external_torque);
  printf("Primary Muscle Force: %.2f N\n", r->F1);
  printf("Secondary Muscle Force: %.2f N\n", r->F2);
  printf("Joint Horizontal Compression: %.2f N\n", r->joint_horizontal);
  printf("Joint Vertical Shear: %.2f N\n", r->joint_vertical);
  printf("Joint Reaction Force: %.2f N\n", r->joint_reaction_total);
}

int main() {
  // --- collect athlete inputs, common to all four skills ---
  athlete a;
  a.gender = getGender();
  a.height = getHeight();
  a.massBody = getWeight();

  int skill = getSkill();

  // --- arm mass/length are needed by every skill, so build them upfront ---
  mass_arm ma = build_mass_arm(a.gender, a.massBody, a.height);
  length_arm la = build_length_arm(a.gender, a.height, &ma);

  // pec/lat is the muscle pair for both iron cross and (half of) victorian,
  // so it's built once here rather than duplicated in both branches below.
  muscle_pair pec_lat = build_pec_lat(la.L_upperArm);

  joint_result r;

  switch (skill) {
  case 1:
    // Iron cross only needs arm mass/length -- no trunk/leg data required.
    r = compute_iron_cross(&a, &ma, &la, &pec_lat);
    print_joint_result("Iron Cross (pec/lat)", &r);
    break;

  case 2: {
    // Maltese additionally needs the trunk+legs lever data.
    lower_body lb = build_lower_body(a.massBody, a.height);
    r = compute_maltese_cross(&a, &ma, &lb, &pec_lat);
    print_joint_result("Maltese Cross (pec/lat)", &r);
    break;
  }

  case 3: {
    // Planche reuses maltese's lever data but needs a different muscle
    // pair (deltoid/serratus instead of pec/lat) -- see compute_planche's
    // comment for why the function body is otherwise identical.
    lower_body lb = build_lower_body(a.massBody, a.height);
    muscle_pair delt_serratus = build_delt_serratus(la.L_upperArm);
    r = compute_planche(&a, &ma, &lb, &delt_serratus);
    print_joint_result("Planche (anterior deltoid/serratus anterior)", &r);
    break;
  }

  case 4: {
    // Victorian needs trunk+legs (for the hip calc) plus a second muscle
    // pair (hip flexors) on top of everything iron cross already needs.
    // It produces two separate joint_results, printed one after the other.
    lower_body lb = build_lower_body(a.massBody, a.height);
    muscle_pair hip_flexors = build_hip_flexors(lb.L_thigh);
    victorian_result v =
        compute_victorian_cross(&a, &ma, &la, &lb, &pec_lat, &hip_flexors);
    print_joint_result("Victorian Cross - Shoulder (pec/lat)", &v.shoulder);
    print_joint_result("Victorian Cross - Hip (iliopsoas/rectus femoris)",
                       &v.hip);
    break;
  }
  }

  return 0;
}
